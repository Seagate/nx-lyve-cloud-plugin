// Copyright © 2024 Seagate Technology LLC and/or its Affiliates
// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "engine.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string>
#include <thread>

#include "actions.h"
#include "active_settings_rules.h"
#include "device_agent.h"
#include "settings_model.h"
#include "stub_analytics_plugin_settings_ini.h"

#include "cloudfuse_helper.h"

#define NX_PRINT_PREFIX (this->logUtils.printPrefix)
#include <nx/kit/debug.h>
#include <nx/sdk/helpers/active_setting_changed_response.h>
#include <nx/sdk/helpers/error.h>
#include <utils.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace settings
{

using namespace nx::sdk;
using namespace nx::sdk::analytics;
using namespace nx::kit;

Engine::Engine(Plugin *plugin)
    : nx::sdk::analytics::Engine(NX_DEBUG_ENABLE_OUTPUT, plugin->instanceId()), m_plugin(plugin), m_cfManager()
{
    NX_PRINT << "cloudfuse Engine::Engine";
    for (const auto &entry : kActiveSettingsRules)
    {
        const ActiveSettingsBuilder::ActiveSettingKey key = entry.first;
        m_activeSettingsBuilder.addRule(key.activeSettingName, key.activeSettingValue,
                                        /*activeSettingHandler*/ entry.second);
    }

    for (const auto &entry : kDefaultActiveSettingsRules)
    {
        m_activeSettingsBuilder.addDefaultRule(
            /*activeSettingName*/ entry.first,
            /*activeSettingHandler*/ entry.second);
    }
}

Engine::~Engine()
{
    NX_PRINT << "cloudfuse Engine::~Engine unmount cloudfuse";
    const processReturn unmountRet = m_cfManager.unmount();
    if (unmountRet.errCode != 0)
    {
        NX_PRINT << "cloudfuse Engine::~Engine failed to unmount cloudfuse with error: " + unmountRet.output;
    }
}

void Engine::doObtainDeviceAgent(Result<IDeviceAgent *> *outResult, const IDeviceInfo *deviceInfo)
{
    NX_PRINT << "cloudfuse Engine::doObtainDeviceAgent";
    *outResult = new DeviceAgent(this, deviceInfo);
}

// functions are not "hoisted", so we need "prototypes"
std::string generatePassphrase();

static std::string buildCapabilities()
{
    std::string capabilities;

    if (ini().deviceDependent)
        capabilities += "|deviceDependent";

    // Delete first '|', if any.
    if (!capabilities.empty() && capabilities.at(0) == '|')
        capabilities.erase(0, 1);

    return capabilities;
}

std::string Engine::manifestString() const
{
    NX_PRINT << "cloudfuse Engine::manifestString";
    std::string result = /*suppress newline*/ 1 + (const char *)R"json(
{
    "capabilities": ")json" +
                         buildCapabilities() + R"json(",
    "deviceAgentSettingsModel":
)json" + kEngineSettingsModel + R"json(
}
)json";

    return result;
}

bool Engine::processActiveSettings(Json::object *model, std::map<std::string, std::string> *values,
                                   const std::vector<std::string> &settingIdsToUpdate) const
{
    NX_PRINT << "cloudfuse Engine::processActiveSettings";
    Json::array items = (*model)[kItems].array_items();

    auto activeSettingsGroupBoxIt = std::find_if(items.begin(), items.end(), [](Json &item) {
        return item[kCaption].string_value() == kAdvancedSettingsGroupBoxCaption;
    });

    if (activeSettingsGroupBoxIt == items.cend())
        return false;

    Json activeSettingsItems = (*activeSettingsGroupBoxIt)[kItems];

    std::vector<std::string> activeSettingNames = settingIdsToUpdate;
    if (activeSettingNames.empty())
    {
        for (const auto &item : activeSettingsItems.array_items())
        {
            if (item["type"].string_value() == "Button")
                continue;

            std::string name = item[kName].string_value();
            activeSettingNames.push_back(name);
        }
    }

    for (const auto &settingId : activeSettingNames)
        m_activeSettingsBuilder.updateSettings(settingId, &activeSettingsItems, values);

    Json::array updatedActiveSettingsItems = activeSettingsItems.array_items();
    Json::object updatedActiveGroupBox = activeSettingsGroupBoxIt->object_items();
    updatedActiveGroupBox[kItems] = updatedActiveSettingsItems;
    *activeSettingsGroupBoxIt = updatedActiveGroupBox;
    (*model)[kItems] = items;

    return true;
}

bool Engine::settingsChanged()
{
    // If cloudfuse is not mounted and settings are the same, then return true so
    // it tries to mount again.
    if (!m_cfManager.isMounted())
    {
        return true;
    }

    std::map<std::string, std::string> newValues = currentSettings();
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
    // nothing we care about changed
    return false;
}

Result<const ISettingsResponse *> Engine::settingsReceived()
{
    NX_PRINT << "cloudfuse Engine::settingsReceived";
    std::string parseError;
    Json::object model = Json::parse(kEngineSettingsModel, parseError).object_items();

    std::map<std::string, std::string> values = currentSettings();

    // check if settings changed
    bool mountRequired = settingsChanged();
    // write new settings to previous
    m_prevSettings = values;
    if (mountRequired)
    {
        NX_PRINT << "Settings changed.";
        mount();
    }
    else
    {
        NX_PRINT << "Settings have not changed.";
    }

    if (!processActiveSettings(&model, &values))
    {
        std::string errorMessage = "Unable to process the active settings section";
        NX_PRINT << errorMessage;
        return error(ErrorCode::internalError, errorMessage);
    }

    auto settingsResponse = new SettingsResponse();
    settingsResponse->setModel(makePtr<String>(Json(model).dump()));
    settingsResponse->setValues(makePtr<StringMap>(values));

    return settingsResponse;
}

bool Engine::mount()
{
    // gather mount options
    getMountOptions();
    // generate the config passphrase
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

void Engine::getMountOptions()
{
    std::map<std::string, std::string> values = currentSettings();
    m_keyId = values[kKeyIdTextFieldId];
    m_secretKey = values[kSecretKeyPasswordFieldId];
    m_endpointUrl = values[kEndpointUrlTextFieldId];
    m_endpointRegion = "us-east-1";
    m_bucketName = values[kBucketNameTextFieldId]; // The default empty string will cause cloudfuse
                                                   // to select first available bucket
    m_mountDir = m_cfManager.getMountDir();
    m_fileCacheDir = m_cfManager.getFileCacheDir();
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

nx::sdk::Error Engine::validateMount()
{
    // Unmount before mounting
    if (fs::exists(m_mountDir))
    {
        const processReturn unmountReturn = m_cfManager.unmount();
        if (unmountReturn.errCode != 0)
        {
            return error(ErrorCode::internalError, "Failed to unmount. Here's why: " + unmountReturn.output);
        }
    }
    std::error_code errCode;
#if defined(__linux__)
    // On Linux the mount folder needs to exist before mounting
    if (!fs::exists(m_mountDir))
    {
        NX_PRINT << "Creating mount directory";
        if (!fs::create_directory(m_mountDir, errCode))
        {
            return error(ErrorCode::internalError, "Unable to create mount directory with error: " + errCode.message());
        }
    }
    // check and set mount folder permissions
    auto mountDirStat = fs::status(m_mountDir);
    if ((mountDirStat.permissions() & fs::perms::all) != fs::perms::all)
    {
        fs::permissions(m_mountDir, fs::perms::all, fs::perm_options::add, errCode);
        if (errCode)
        {
            return error(ErrorCode::internalError,
                         "Unable to set mount directory permissions with error: " + errCode.message());
        }
    }
#endif
    // Create file cache if it does not exist
    if (!fs::exists(m_fileCacheDir))
    {
        NX_PRINT << "creating file cache since it does not exist";
        if (!fs::create_directories(m_fileCacheDir, errCode))
        {
            return error(ErrorCode::internalError, "Unable to create file cache directory " + m_fileCacheDir +
                                                       " with error: " + errCode.message());
        }
    }
    // check and set file cache directory permissions
    auto fileCacheDirStat = fs::status(m_fileCacheDir);
    if ((fileCacheDirStat.permissions() & fs::perms::all) != fs::perms::all)
    {
        fs::permissions(m_fileCacheDir, fs::perms::all, fs::perm_options::add, errCode);
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
        m_cfManager.genS3Config(m_endpointRegion, m_endpointUrl, m_bucketName, m_passphrase);
#elif defined(_WIN32)
    const processReturn dryGenConfig =
        m_cfManager.genS3Config(m_keyId, m_secretKey, m_endpointRegion, m_endpointUrl, m_bucketName, m_passphrase);
#endif
    if (dryGenConfig.errCode != 0)
    {
        return error(ErrorCode::internalError, "Unable to generate config file with error: " + dryGenConfig.output);
    }

    // do a dry run to verify user credentials
    NX_PRINT << "Checking cloud credentials (cloudfuse dry run)";
#if defined(__linux__)
    const processReturn dryRunRet = m_cfManager.dryRun(m_keyId, m_secretKey, m_passphrase);
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
    return error(ErrorCode::noError, nullptr);
}

nx::sdk::Error Engine::spawnMount()
{
    // mount the bucket
    NX_PRINT << "Starting cloud storage mount";
#if defined(__linux__)
    const processReturn mountRet = m_cfManager.mount(m_keyId, m_secretKey, m_passphrase);
#elif defined(_WIN32)
    const processReturn mountRet = m_cfManager.mount(m_passphrase);
#endif
    if (mountRet.errCode != 0)
    {
        std::string errorMessage = "Unable to launch mount with error: " + mountRet.output;
        NX_PRINT << errorMessage;
        return error(ErrorCode::internalError, errorMessage);
    }

    // Mount might not show up immediately, so wait for mount to appear
    int maxRetries = 10;
    for (int retryCount = 0; retryCount < maxRetries; retryCount++)
    {
        if (m_cfManager.isMounted())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (!m_cfManager.isMounted())
    {
        std::string errorMessage = "Cloudfuse was not able to successfully mount";
        NX_PRINT << errorMessage;
        return error(ErrorCode::internalError, errorMessage);
    }

    return error(ErrorCode::noError, nullptr);
}

void Engine::getPluginSideSettings(Result<const ISettingsResponse *> *outResult) const
{
    NX_PRINT << "cloudfuse Engine::getPluginSideSettings";
    auto settingsResponse = new SettingsResponse();
    settingsResponse->setValue(kEnginePluginSideSetting, kEnginePluginSideSettingValue);

    *outResult = settingsResponse;
}

void Engine::doGetSettingsOnActiveSettingChange(Result<const IActiveSettingChangedResponse *> *outResult,
                                                const IActiveSettingChangedAction *activeSettingChangedAction)
{
    NX_PRINT << "cloudfuse Engine::doGetSettingsOnActiveSettingChange";
    std::string parseError;
    Json::object model = Json::parse(activeSettingChangedAction->settingsModel(), parseError).object_items();

    const std::string settingId(activeSettingChangedAction->activeSettingName());

    std::map<std::string, std::string> values = toStdMap(shareToPtr(activeSettingChangedAction->settingsValues()));

    if (!processActiveSettings(&model, &values, {settingId}))
    {
        std::string errorMessage = "Unable to process the active settings section";
        NX_PRINT << errorMessage;
        *outResult = error(ErrorCode::internalError, errorMessage);

        return;
    }

    const auto settingsResponse = makePtr<SettingsResponse>();
    settingsResponse->setValues(makePtr<StringMap>(values));
    settingsResponse->setModel(makePtr<String>(Json(model).dump()));

    const nx::sdk::Ptr<nx::sdk::ActionResponse> actionResponse =
        generateActionResponse(settingId, activeSettingChangedAction->params());

    auto response = makePtr<ActiveSettingChangedResponse>();
    response->setSettingsResponse(settingsResponse);
    response->setActionResponse(actionResponse);

    *outResult = response.releasePtr();
}

} // namespace settings
