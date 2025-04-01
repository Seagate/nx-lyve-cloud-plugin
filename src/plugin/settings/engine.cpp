// Copyright © 2024 Seagate Technology LLC and/or its Affiliates
// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "engine.h"

#include <algorithm>
#include <chrono>
#include <codecvt>
#include <filesystem>
#include <fstream>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string>
#include <thread>

#include "device_agent.h"
#include "settings_model.h"
#include "stub_analytics_plugin_settings_ini.h"

// TODO: get NX_PRINT_PREFIX working with non-member helper functions
// #define NX_PRINT_PREFIX (this->logUtils.printPrefix)
#include <nx/kit/debug.h>
#include <nx/kit/ini_config.h>
#include <nx/sdk/helpers/active_setting_changed_response.h>
#include <nx/sdk/helpers/error.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace settings
{

using namespace nx::sdk;
using namespace nx::sdk::analytics;
using namespace nx::kit;

static bool checkSaasSubscription();
static std::string buildCapabilities();
static std::string generatePassphrase();
static void enableLogging(std::string iniDir);
static std::string parseCloudfuseError(std::string error);

static int maxWaitSecondsAfterMount = 10;

Engine::Engine(Plugin *plugin)
    : nx::sdk::analytics::Engine(NX_DEBUG_ENABLE_OUTPUT, plugin->instanceId()), m_plugin(plugin), m_cfManager()
{
    NX_PRINT << "cloudfuse Engine::Engine";
    // check SaaS subscription
    m_saasSubscriptionValid = checkSaasSubscription();
}

Engine::~Engine()
{
    NX_PRINT << "cloudfuse Engine::~Engine unmount cloudfuse";
    const processReturn unmountRet = m_cfManager.unmount();
    if (unmountRet.errCode != 0)
    {
        NX_PRINT << "cloudfuse Engine::~Engine failed to unmount cloudfuse with error: " + unmountRet.output;
    }
    // enable logging for the _next_ time the mediaserver starts
    enableLogging(IniConfig::iniFilesDir());
}

std::string Engine::manifestString() const
{
    NX_PRINT << "cloudfuse Engine::manifestString";
    std::string result = /*suppress newline*/ 1 + (const char *)R"json(
{
    "capabilities": ")json" +
                         buildCapabilities() + R"json(",
    "deviceAgentSettingsModel":
)json" + kEngineSettingsModel +
                         R"json(
}
)json";

    return result;
}

Result<const ISettingsResponse *> Engine::settingsReceived()
{
    NX_PRINT << "cloudfuse Engine::settingsReceived";
    std::string parseError;
    Json::object model = Json::parse(kEngineSettingsModel, parseError).object_items();
    if (parseError != "")
    {
        std::string errorMessage = "Failed to parse engine settings model. Here's why:" + parseError;
        NX_PRINT << errorMessage;
        return error(ErrorCode::internalError, errorMessage);
    }

    // first, let the user know whether this plugin is authorized by an active SaaS subscription
    auto subscriptionStatusJson = m_saasSubscriptionValid ? kStatusSaaSSubscriptionVerified : kStatusNoSaaSSubscription;
    if (!setStatusBanner(&model, kSubscriptionStatusBannerId, subscriptionStatusJson))
    {
        // on failure, no changes will be written to the model
        NX_PRINT << "SaaS subscription status message update failed!";
    }

    std::map<std::string, std::string> values = currentSettings();

    // check if settings changed
    bool mountRequired = settingsChanged();
    // write new settings to previous
    m_prevSettings = values;
    bool mountSuccessful;
    if (mountRequired)
    {
        NX_PRINT << "Settings changed.";
        mountSuccessful = mount();
        if (!mountSuccessful)
        {
            NX_PRINT << "Mount failed.";
        }
    }
    else
    {
        NX_PRINT << "Settings have not changed.";
        mountSuccessful = m_cfManager.isMounted();
    }
    // update the model so user can see mount status
    auto statusJson = mountSuccessful ? kStatusSuccess : kStatusFailure;
    if (!setStatusBanner(&model, kBucketStatusBannerId, statusJson))
    {
        // on failure, no changes will be written to the model
        NX_PRINT << "Status message update failed!";
    }

    // returning invalid JSON to the VMS will crash the server
    // validate JSON before sending.
    NX_PRINT << "Returning settingsResponse...";
    auto settingValuesMap = makePtr<StringMap>(values);
    std::map<std::string, std::string> validSettings;
    if (!logUtils.convertAndOutputStringMap(&validSettings, settingValuesMap.get(), "Validating settingsResponse"))
    {
        std::string errorMessage = "SettingsResponse - invalid return JSON";
        NX_PRINT << errorMessage;
        return error(ErrorCode::internalError, errorMessage);
    }

    auto settingsResponse = new SettingsResponse();
    settingsResponse->setModel(makePtr<String>(Json(model).dump()));
    settingsResponse->setValues(settingValuesMap);
    return settingsResponse;
}

bool Engine::mount()
{
    // generate the config passphrase
    NX_PRINT << "Generating passphrase...";
    m_passphrase = generatePassphrase();
    if (m_passphrase == "")
    {
        std::string errorMessage = "OpenSSL Error: Unable to generate secure passphrase";
        NX_PRINT << errorMessage;
        pushPluginDiagnosticEvent(IPluginDiagnosticEvent::Level::error, "OpenSSL Error", errorMessage);
        return false;
    }

    auto validationErr = validateMount();
    if (!validationErr.isOk())
    {
        std::string errorMessage =
            "mount aborted (validation failed). Here's why: " + std::string(Ptr(validationErr.errorMessage())->str());
        NX_PRINT << errorMessage;
        pushPluginDiagnosticEvent(IPluginDiagnosticEvent::Level::error, "Cloud Storage Connection Error", errorMessage);
        return false;
    }

    auto mountErr = spawnMount();
    if (!mountErr.isOk())
    {
        // mount failed - this is very unexpected
        std::string errorMessage = "mount failed. Here's why: " + std::string(Ptr(mountErr.errorMessage())->str());
        NX_PRINT << errorMessage;
        pushPluginDiagnosticEvent(IPluginDiagnosticEvent::Level::error, "Cloud Storage Mount Error", errorMessage);
        return false;
    }

    return true;
}

void Engine::doObtainDeviceAgent(Result<IDeviceAgent *> *outResult, const IDeviceInfo *deviceInfo)
{
    NX_PRINT << "cloudfuse Engine::doObtainDeviceAgent";
    *outResult = new DeviceAgent(this, deviceInfo);
}

void Engine::getPluginSideSettings(Result<const ISettingsResponse *> *outResult) const
{
    NX_PRINT << "cloudfuse Engine::getPluginSideSettings";
    auto settingsResponse = new SettingsResponse();
    *outResult = settingsResponse;
}

void Engine::doGetSettingsOnActiveSettingChange(Result<const IActiveSettingChangedResponse *> *outResult,
                                                const IActiveSettingChangedAction *activeSettingChangedAction)
{
    NX_PRINT << "cloudfuse Engine::doGetSettingsOnActiveSettingChange";
}

bool Engine::settingsChanged()
{
    // pull settings
    std::map<std::string, std::string> newValues = currentSettings();

    // check if settings are empty
    if (newValues[kKeyIdTextFieldId] == "" && newValues[kSecretKeyPasswordFieldId] == "")
    {
        NX_PRINT << "Settings are empty. Ignoring...";
        return false;
    }

    // If cloudfuse is not mounted and settings are the same, then return true so
    // it tries to mount again.
    if (!m_cfManager.isMounted())
    {
        return true;
    }

    // if we're mounted and the settings haven't changed, do nothing
    if (newValues == m_prevSettings)
    {
        return false;
    }

    // we only really care about certain values
    // key ID
    if (m_prevSettings[kKeyIdTextFieldId] != newValues[kKeyIdTextFieldId])
    {
        return true;
    }
    // secret key
    if (m_prevSettings[kSecretKeyPasswordFieldId] != newValues[kSecretKeyPasswordFieldId])
    {
        return true;
    }
    if (!credentialsOnly)
    {
        // endpoint
        if (m_prevSettings[kEndpointUrlTextFieldId] != newValues[kEndpointUrlTextFieldId])
        {
            // if they're different, but both amount to the same thing, then there is no effective change
            bool prevIsDefault = m_prevSettings[kEndpointUrlTextFieldId] == kDefaultEndpoint ||
                                 m_prevSettings[kEndpointUrlTextFieldId] == "";
            bool newIsDefault =
                newValues[kEndpointUrlTextFieldId] == kDefaultEndpoint || newValues[kEndpointUrlTextFieldId] == "";
            if (!prevIsDefault || !newIsDefault)
            {
                return true;
            }
        }
        // bucket name
        if (m_prevSettings[kBucketNameTextFieldId] != newValues[kBucketNameTextFieldId])
        {
            return true;
        }
        // bucket capacity
        if (m_prevSettings[kBucketSizeTextFieldId] != newValues[kBucketSizeTextFieldId])
        {
            return true;
        }
    }
    // nothing we care about changed
    return false;
}

nx::sdk::Error Engine::validateMount()
{
    NX_PRINT << "Validating mount options...";
    std::map<std::string, std::string> values = currentSettings();
    std::string keyId = values[kKeyIdTextFieldId];
    std::string secretKey = values[kSecretKeyPasswordFieldId];
    std::string endpointUrl = kDefaultEndpoint;
    std::string bucketName = "";
    uint64_t bucketCapacityGB = kDefaultBucketSizeGb;
    if (!credentialsOnly)
    {
        endpointUrl = values[kEndpointUrlTextFieldId];
        bucketName = values[kBucketNameTextFieldId]; // The default empty string will cause cloudfuse
                                                     // to select first available bucket
        try
        {
            bucketCapacityGB = std::stoi(values[kBucketSizeTextFieldId]);
        }
        catch (std::invalid_argument &e)
        {
            NX_PRINT << "Bad input for bucket capacity: " << values[kBucketSizeTextFieldId];
            // revert to default - write back to settings so user can see value reset
            values[kBucketSizeTextFieldId] = std::to_string(kDefaultBucketSizeGb);
        }
    }
    std::string mountDir = m_cfManager.getMountDir();
    std::string fileCacheDir = m_cfManager.getFileCacheDir();
    // Unmount before mounting
    std::error_code errCode;
    if (m_cfManager.isMounted())
    {
        NX_PRINT << "Bucket is mounted. Unmounting...";
        const processReturn unmountReturn = m_cfManager.unmount();
        if (unmountReturn.errCode != 0)
        {
            return error(ErrorCode::internalError, "Failed to unmount. Here's why: " + unmountReturn.output);
        }
#if defined(_WIN32)
        for (int s = 0; s < maxWaitSecondsAfterMount; s++)
        {
            if (!fs::exists(mountDir, errCode) && !(bool)errCode)
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (fs::exists(mountDir, errCode) || (bool)errCode)
        {
            return error(ErrorCode::internalError,
                         "Unmount failed - " + std::to_string(maxWaitSecondsAfterMount) + "s timeout reached.");
        }
#endif
    }
#if defined(__linux__)
    // On Linux the mount folder needs to exist before mounting
    if (!fs::exists(mountDir))
    {
        NX_PRINT << "Creating mount directory";
        if (!fs::create_directory(mountDir, errCode))
        {
            return error(ErrorCode::internalError, "Unable to create mount directory with error: " + errCode.message());
        }
    }
    // check and set mount folder permissions
    auto mountDirStat = fs::status(mountDir);
    if ((mountDirStat.permissions() & fs::perms::all) != fs::perms::all)
    {
        fs::permissions(mountDir, fs::perms::all, fs::perm_options::add, errCode);
        if (errCode)
        {
            return error(ErrorCode::internalError,
                         "Unable to set mount directory permissions with error: " + errCode.message());
        }
    }
#endif
    // Create file cache if it does not exist
    if (!fs::exists(fileCacheDir))
    {
        NX_PRINT << "creating file cache since it does not exist";
        if (!fs::create_directories(fileCacheDir, errCode))
        {
            return error(ErrorCode::internalError,
                         "Unable to create file cache directory " + fileCacheDir + " with error: " + errCode.message());
        }
    }
    // check and set file cache directory permissions
    auto fileCacheDirStat = fs::status(fileCacheDir);
    if ((fileCacheDirStat.permissions() & fs::perms::all) != fs::perms::all)
    {
        fs::permissions(fileCacheDir, fs::perms::all, fs::perm_options::add, errCode);
        if (errCode)
        {
            return error(ErrorCode::internalError,
                         "Unable to set file cache directory permission with error: " + errCode.message());
        }
    }

    // generate cloudfuse config
    if (!m_cfManager.isInstalled())
    {
        return error(ErrorCode::internalError, "Cloudfuse is not installed");
    }
    NX_PRINT << "spawning process from genS3Config";
#if defined(__linux__)
    const processReturn dryGenConfig =
        m_cfManager.genS3Config(endpointUrl, bucketName, bucketCapacityGB * 1024, m_passphrase);
#elif defined(_WIN32)
    const processReturn dryGenConfig =
        m_cfManager.genS3Config(keyId, secretKey, endpointUrl, bucketName, bucketCapacityGB * 1024, m_passphrase);
#endif
    if (dryGenConfig.errCode != 0)
    {
        return error(ErrorCode::internalError, "Unable to generate config file with error: " + dryGenConfig.output);
    }

    // do a dry run to verify user credentials
    NX_PRINT << "Checking cloud credentials (cloudfuse dry run)";
#if defined(__linux__)
    const processReturn dryRunRet = m_cfManager.dryRun(keyId, secretKey, m_passphrase);
#elif defined(_WIN32)
    const processReturn dryRunRet = m_cfManager.dryRun(m_passphrase);
#endif
    // possible error codes from dry run:
    std::array<std::string, 4> errorMatchStrings{"Bucket Error", "Credential or Endpoint Error", "Endpoint Error",
                                                 "Secret Error"};
    std::array<std::string, 4> errorMessageStrings{"Unable to authenticate with bucket",
                                                   "Error with cloud credentials or incorrect endpoint",
                                                   "Error with provided endpoint", "Secret key provided is incorrect"};
    if (dryRunRet.errCode != 0)
    {
        for (size_t i = 0; i < errorMatchStrings.size(); i++)
        {
            if (dryRunRet.output.find(errorMatchStrings[i]) != std::string::npos)
            {
                return error(ErrorCode::invalidParams,
                             errorMessageStrings[i] + ": " + parseCloudfuseError(dryRunRet.output));
            }
        }
        // Otherwise this is an error we did not prepare for
        return error(ErrorCode::internalError,
                     "Unable to validate credentials with error: " + parseCloudfuseError(dryRunRet.output));
    }
    return Error(ErrorCode::noError, nullptr);
}

nx::sdk::Error Engine::spawnMount()
{
    std::map<std::string, std::string> values = currentSettings();
    std::string keyId = values[kKeyIdTextFieldId];
    std::string secretKey = values[kSecretKeyPasswordFieldId];
    // mount the bucket
    NX_PRINT << "Starting cloud storage mount";
#if defined(__linux__)
    const processReturn mountRet = m_cfManager.mount(keyId, secretKey, m_passphrase);
#elif defined(_WIN32)
    const processReturn mountRet = m_cfManager.mount(m_passphrase);
#endif
    if (mountRet.errCode != 0)
    {
        return error(ErrorCode::internalError, "Unable to launch mount with error: " + mountRet.output);
    }

    // Mount might not show up immediately, so wait for mount to appear
    for (int s = 0; s < maxWaitSecondsAfterMount; s++)
    {
        if (m_cfManager.isMounted())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (!m_cfManager.isMounted())
    {
        return error(ErrorCode::internalError, "Cloudfuse was not able to successfully mount");
    }

    return Error(ErrorCode::noError, nullptr);
}

bool Engine::setStatusBanner(Json::object *model, std::string bannerId, std::string updatedJson) const
{
    NX_PRINT << "cloudfuse Engine::setStatusBanner " << bannerId;

    // prepare the new status item
    std::string error;
    auto newStatus = Json::parse(updatedJson, error);
    if (error != "")
    {
        NX_PRINT << "Failed to parse status JSON with error: " << error;
        return false;
    }

    // find where to put it
    auto items = (*model)[kItems];
    auto itemsArray = items.array_items();
    // find the status banner, if it's already present
    auto statusBannerIt = std::find_if(itemsArray.begin(), itemsArray.end(),
                                       [bannerId](Json &item) { return item[kName].string_value() == bannerId; });
    // if the banner is not there, add it
    if (statusBannerIt == itemsArray.end())
    {
        // add the status banner to the beginning of the list of items
        itemsArray.insert(itemsArray.begin(), newStatus);
    }
    else
    {
        // update the status
        *statusBannerIt = newStatus;
    }

    // write the updated array back into the model Json
    // TODO: why do we have to do this?
    (*model)[kItems] = itemsArray;

    return true;
}

// static helper functions

void enableLogging(std::string iniDir)
{
    // enable logging by touching stdout and stderr redirect files
    if (!fs::is_directory(iniDir))
    {
        fs::create_directories(iniDir);
    }
    const std::string processName = utils::getProcessName();
    const std::string stdoutFilename = iniDir + processName + "_stdout.log";
    const std::string stderrFilename = iniDir + processName + "_stderr.log";
    if (!fs::exists(stdoutFilename) || !fs::exists(stderrFilename))
    {
        NX_PRINT << "cloudfuse Engine::enableLogging - creating files";
        std::ofstream stdoutFile(stdoutFilename);
        std::ofstream stderrFile(stderrFilename);
        // the service will need to be restarted for logging to actually begin
    }
    NX_PRINT << "cloudfuse Engine::enableLogging - plugin stderr logging file: " + stderrFilename;
}

std::string buildCapabilities()
{
    std::string capabilities;

    if (ini().deviceDependent)
        capabilities += "|deviceDependent";

    // Delete first '|', if any.
    if (!capabilities.empty() && capabilities.at(0) == '|')
        capabilities.erase(0, 1);

    return capabilities;
}

std::string generatePassphrase()
{
    // Generate passphrase for config file
    unsigned char key[32]; // AES-256 key
    if (!RAND_bytes(key, sizeof(key)))
    {
        return "";
    }
    // Need to encode passphrase to base64 to pass to cloudfuse
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, key, 32);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    return std::string(bptr->data, bptr->length);
}

// parseCloudfuseError takes in an error and trims the error down to it's most essential
// error, which from cloudfuse is the error returned between braces []
std::string parseCloudfuseError(std::string error)
{
    std::size_t start = error.find_last_of('[');
    if (start == std::string::npos)
    {
        return error;
    }

    std::size_t end = error.find_last_of(']');
    if (end == std::string::npos)
    {
        return error;
    }

    return error.substr(start, end);
}

processReturn getServerPort()
{
#if defined(_WIN32)
    wchar_t systemRoot[MAX_PATH];
    GetEnvironmentVariableW(L"SystemRoot", systemRoot, MAX_PATH);
    const std::wstring regPath = std::wstring(systemRoot) + LR"(\system32\reg.exe)";
    const std::string registryKey = R"(HKEY_LOCAL_MACHINE\SOFTWARE\Network Optix\Network Optix MetaVMS Media Server)";
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wRegistryKey = converter.from_bytes(registryKey);
    const std::wstring wargv = regPath + L" query \"" + wRegistryKey + L"\" /v port";
    const std::wstring wenvp = L"";
    auto processReturn = ChildProcess::spawnProcess(const_cast<wchar_t *>(wargv.c_str()), wenvp);
    // return on error (handled by the caller)
    if (processReturn.errCode != 0)
    {
        return processReturn;
    }
    // parse the port number from the registry query output
    std::stringstream textStream(processReturn.output);
    std::string line;
    while (std::getline(textStream, line))
    {
        // drop \r
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        // skip empty lines, and skip the line repeating the registry key
        if (line.empty() || line == registryKey)
        {
            continue;
        }
        // split the line we're interested (example below) by spaces:
        // "    port    REG_SZ    7001"
        std::stringstream lineStream(line);
        std::string token;
        while (lineStream >> token)
        {
            // skip until we get to the port number
            if (token.empty() || !std::all_of(token.begin(), token.end(), ::isdigit))
            {
                continue;
            }
            // found the port number
            // overwrite the output to the port, and return
            processReturn.output = token;
            return processReturn;
        }
    }
    // port not found
    processReturn.errCode = 1;
#elif defined(__linux__)
    // grep port /opt/networkoptix-metavms/mediaserver/etc/mediaserver.conf
    const std::string vmsConfigPath = "/opt/networkoptix-metavms/mediaserver/etc/mediaserver.conf";
    char *const argv[] = {const_cast<char *>("/usr/bin/sed"), const_cast<char *>("-rn"),
                          const_cast<char *>("s/port=([0-9]*)/\\1/p"), const_cast<char *>(vmsConfigPath.c_str()), NULL};
    char *const envp[] = {NULL};
    auto processReturn = ChildProcess::spawnProcess(argv, envp);
#endif
    return processReturn;
}

Json getMediaserverSystemInfo(const std::string port, const std::string apiVersion)
{
    // use curl to make a request for system info
    const std::string vmsSystemInfoUrl = "https://localhost:" + port + "/rest/v" + apiVersion + "/system/info";
#if defined(_WIN32)
    // convert URL to wstring
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wVmsSystemInfoUrl = converter.from_bytes(vmsSystemInfoUrl);
    // get curl path so we don't need environment variables
    wchar_t systemRoot[MAX_PATH];
    GetEnvironmentVariableW(L"SystemRoot", systemRoot, MAX_PATH);
    const std::wstring curlPath = std::wstring(systemRoot) + LR"(\system32\curl.exe)";
    const std::wstring wargv = curlPath + L" -k -m 5" + wVmsSystemInfoUrl;
    const std::wstring wenvp = L"";
    auto processReturn = ChildProcess::spawnProcess(const_cast<wchar_t *>(wargv.c_str()), wenvp);
#elif defined(__linux__)
    char *const argv[] = {const_cast<char *>("/usr/bin/curl"),
                          const_cast<char *>("-k"),
                          const_cast<char *>("-m"),
                          const_cast<char *>("5"),
                          const_cast<char *>(vmsSystemInfoUrl.c_str()),
                          NULL};
    char *const envp[] = {NULL};
    auto processReturn = ChildProcess::spawnProcess(argv, envp);
#endif
    if (processReturn.errCode != 0)
    {
        NX_PRINT << "cloudfuse Engine::Engine Failed to get media server system information. Here's why: "
                 << processReturn.output;
        return Json();
    }
    // try to parse JSON data
    std::string parseError;
    auto systemInfo = Json::parse(processReturn.output, parseError);
    if (!parseError.empty())
    {
        NX_PRINT << "Failed to parse media server system info JSON. Here's why: " + parseError;
        return false;
    }
    // check for API error
    if (systemInfo["error"].is_string())
    {
        NX_PRINT << "Media server API error: " + systemInfo.dump();
        // I've seen this as an API version mismatch error - retry with version 2
        if (apiVersion == "3")
        {
            return getMediaserverSystemInfo(port, "2");
        }
    }

    return systemInfo;
}

bool checkSaasSubscription()
{
    // first, get the server port number
    auto portProcessReturn = getServerPort();
    if (portProcessReturn.errCode != 0)
    {
        NX_PRINT << "cloudfuse Engine::Engine Failed to get media server port number. Here's why: "
                 << portProcessReturn.output;
        return false;
    }
    std::string port = portProcessReturn.output;
    // strip endline(s) and check that the port is numeric
    while (!port.empty() && (port.back() == '\n' || port.back() == '\r'))
    {
        port.erase(port.size() - 1);
    }
    if (port.empty() || !std::all_of(port.begin(), port.end(), ::isdigit))
    {
        NX_PRINT << "unexpected non-numeric media server port number: " << port;
        return false;
    }
    // get server system info
    auto systemInfo = getMediaserverSystemInfo(port, "3");
    if (systemInfo.is_null())
    {
        return false;
    }
    auto orgId = systemInfo["organizationId"];
    if (orgId.is_string() && !orgId.string_value().empty())
    {
        NX_PRINT << "found organizationId";
        return true;
    }
    else
    {
        NX_PRINT << "organizationId field not found in mediaserver system information";
        return false;
    }
}

} // namespace settings
