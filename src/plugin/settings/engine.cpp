// Copyright © 2024 Seagate Technology LLC and/or its Affiliates
// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "engine.h"

#include <algorithm>
#include <chrono>
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

static std::string buildCapabilities();
static std::string generatePassphrase();
static void enableLogging(std::string iniDir);
static std::string parseCloudfuseError(std::string error);

static int maxWaitSecondsAfterMount = 10;

Engine::Engine(Plugin *plugin)
    : nx::sdk::analytics::Engine(NX_DEBUG_ENABLE_OUTPUT, plugin->instanceId()), m_plugin(plugin), m_cfManager()
{
    NX_PRINT << "cloudfuse Engine::Engine";
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

    std::map<std::string, std::string> values = currentSettings();

    // check if settings changed
    bool mountRequired = settingsChanged();
    // write new settings to previous
    m_prevSettings = values;
    if (mountRequired)
    {
        NX_PRINT << "Settings changed.";
        bool mountSuccessful = mount();
        if (!mountSuccessful)
        {
            NX_PRINT << "Mount failed.";
        }
        // update the model so user can see mount status
        if (!updateModel(&model, mountSuccessful))
        {
            // on failure, no changes will be written to the model
            NX_PRINT << "Status message update failed!";
        }
    }
    else
    {
        NX_PRINT << "Settings have not changed.";
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
    // have the settings changed?
    std::map<std::string, std::string> newValues = currentSettings();
    if (newValues == m_prevSettings)
    {
        return false;
    }

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

bool Engine::updateModel(Json::object *model, bool mountSuccessful) const
{
    NX_PRINT << "cloudfuse Engine::updateModel";

    // prepare the new status item
    auto statusJson = mountSuccessful ? kStatusSuccess : kStatusFailure;
    std::string error;
    auto newStatus = Json::parse(statusJson, error);
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
                                       [](Json &item) { return item[kName].string_value() == kStatusBannerId; });
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

} // namespace settings
