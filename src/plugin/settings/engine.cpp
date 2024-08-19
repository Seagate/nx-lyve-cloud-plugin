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
#include <string.h>
#include <thread>

#include "actions.h"
#include "active_settings_rules.h"
#include "device_agent.h"
#include "settings_model.h"
#include "stub_analytics_plugin_settings_ini.h"

#include "cloudfuse_helper.h"

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
    : nx::sdk::analytics::Engine(NX_DEBUG_ENABLE_OUTPUT, plugin->instanceId()), m_plugin(plugin), cfManager()
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
    const processReturn unmountRet = cfManager.unmount();
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
)json" + kEngineSettingsModel +
                         R"json(
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
    if (!cfManager.isMounted())
    {
        return true;
    }

    std::map<std::string, std::string> prevValues = prevSettings();
    std::map<std::string, std::string> newValues = currentSettings();
    if (newValues == prevValues)
    {
        return false;
    }
    // we only really care about certain values
    // key ID
    if (prevValues[kKeyIdTextFieldId] != newValues[kKeyIdTextFieldId])
    {
        return true;
    }
    // secret key
    if (prevValues[kSecretKeyPasswordFieldId] != newValues[kSecretKeyPasswordFieldId])
    {
        return true;
    }
    // endpoint
    if (prevValues[kEndpointUrlTextFieldId] != newValues[kEndpointUrlTextFieldId])
    {
        // if they're different, but both amount to the same thing, then there is no effective change
        bool prevIsDefault =
            prevValues[kEndpointUrlTextFieldId] == kDefaultEndpoint || prevValues[kEndpointUrlTextFieldId] == "";
        bool newIsDefault =
            newValues[kEndpointUrlTextFieldId] == kDefaultEndpoint || newValues[kEndpointUrlTextFieldId] == "";
        if (!prevIsDefault || !newIsDefault)
        {
            return true;
        }
    }
    // bucket name
    if (prevValues[kBucketNameTextFieldId] != newValues[kBucketNameTextFieldId])
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
    updatePrevSettings(values);
    if (mountRequired)
    {
        NX_PRINT << "Values changed";
        const std::string keyId = values[kKeyIdTextFieldId];
        const std::string secretKey = values[kSecretKeyPasswordFieldId];
        const std::string endpointUrl = values[kEndpointUrlTextFieldId];
        const std::string endpointRegion = "us-east-1";
        const std::string bucketName = values[kBucketNameTextFieldId]; // The default empty string will cause cloudfuse
                                                                       // to select first available bucket
        const std::string mountDir = cfManager.getMountDir();
        const std::string fileCacheDir = cfManager.getFileCacheDir();
        std::string passphrase = "";
        // Generate passphrase for config file
        unsigned char key[32]; // AES-256 key
        if (RAND_bytes(key, sizeof(key)))
        {
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

            passphrase = std::string(bptr->data, bptr->length);
        }
        else
        {
            NX_PRINT << "OpenSSL Error: Unable to generate secure passphrase";
            return error(ErrorCode::internalError, "OpenSSL Error: Unable to generate secure passphrase");
        }

        std::error_code errCode;

        // Create mount directory if it does not exist
        NX_PRINT << "Creating mount directory if it does not exist";
        if (fs::exists(mountDir))
        {
            // Unmount before mounting
            const processReturn unmountReturn = cfManager.unmount();
            if (unmountReturn.errCode != 0)
            {
                return error(ErrorCode::internalError, "Failed to unmount. Here's why: " + unmountReturn.output);
            }

#if defined(__linux__)
            // On Linux we need to check the folder has the correct permissions
            fs::file_status s = fs::status(mountDir);
            if ((s.permissions() & fs::perms::all) != fs::perms::all)
            {
                fs::permissions(mountDir, fs::perms::all, fs::perm_options::add, errCode);
                if (errCode)
                {
                    NX_PRINT << "Unable to set mount directory permission with error: " + errCode.message();
                    return error(ErrorCode::internalError,
                                 "Unable to set mount directory permission with error: " + errCode.message());
                }
            }
#endif
        }
        else
        {
#if defined(__linux__)
            // On Linux we need to create the folder if it does not yet exist
            NX_PRINT << "creating the folder since it does not yet exist";
            if (!fs::create_directory(mountDir, errCode))
            {
                NX_PRINT << "Unable to create mount directory with error: " + errCode.message();
                return error(ErrorCode::internalError,
                             "Unable to create mount directory with error: " + errCode.message());
            }
            fs::permissions(mountDir, fs::perms::all, fs::perm_options::add, errCode);
            if (errCode)
            {
                NX_PRINT << "Unable to set mount directory permissions with error: " + errCode.message();
                return error(ErrorCode::internalError,
                             "Unable to set mount directory permissions with error: " + errCode.message());
            }
#endif
        }

        // Create file cache if it does not exist
        if (fs::exists(fileCacheDir))
        {
            fs::file_status s = fs::status(fileCacheDir);
            if ((s.permissions() & fs::perms::all) != fs::perms::all)
            {
                fs::permissions(fileCacheDir, fs::perms::all, fs::perm_options::add, errCode);
                if (errCode)
                {
                    NX_PRINT << "Unable to set mount directory permission with error: " + errCode.message();
                    return error(ErrorCode::internalError,
                                 "Unable to set mount directory permission with error: " + errCode.message());
                }
            }
        }
        else
        {
            NX_PRINT << "creating file cache since it does not exist";
            if (!fs::create_directories(fileCacheDir, errCode))
            {
                NX_PRINT << "Unable to create file cache directory " + fileCacheDir +
                                " with error: " + errCode.message();
                return error(ErrorCode::internalError, "Unable to create file cache directory " + fileCacheDir +
                                                           " with error: " + errCode.message());
            }
            fs::permissions(fileCacheDir, fs::perms::all, fs::perm_options::add, errCode);
            if (errCode)
            {
                NX_PRINT << "Unable to set file cache directory permissions with error: " + errCode.message();
                return error(ErrorCode::internalError,
                             "Unable to set file cache directory permissions with error: " + errCode.message());
            }
        }

        if (!cfManager.isInstalled())
        {
            NX_PRINT << "Cloudfuse is not installed";
            return error(ErrorCode::internalError, "Cloudfuse is not installed");
        }
        NX_PRINT << "spawning process from genS3Config";
#if defined(__linux__)
        const processReturn dryGenConfig = cfManager.genS3Config(endpointRegion, endpointUrl, bucketName, passphrase);
#elif defined(_WIN32)
        const processReturn dryGenConfig =
            cfManager.genS3Config(keyId, secretKey, endpointRegion, endpointUrl, bucketName, passphrase);
#endif

        if (dryGenConfig.errCode != 0)
        {
            NX_PRINT << "Unable to generate config file with error: " + dryGenConfig.output;
            return error(ErrorCode::internalError, "Unable to generate config file with error: " + dryGenConfig.output);
        }
        NX_PRINT << "spawning process from dryRun";
#if defined(__linux__)
        const processReturn dryRunRet = cfManager.dryRun(keyId, secretKey, passphrase);
#elif defined(_WIN32)
        const processReturn dryRunRet = cfManager.dryRun(passphrase);
#endif

        if (dryRunRet.errCode != 0)
        {
            if (dryRunRet.output.find("Bucket Error") != std::string::npos)
            {
                Engine::pushPluginDiagnosticEvent(IPluginDiagnosticEvent::Level::error, "Plugin Bucket Error",
                                                  "Error with provided cloud bucket: " +
                                                      parseCloudfuseError(dryRunRet.output));
                NX_PRINT << "Unable to authenticate with bucket: " + parseCloudfuseError(dryRunRet.output);
                return error(ErrorCode::otherError,
                             "Unable to authenticate with bucket: " + parseCloudfuseError(dryRunRet.output));
            }
            if (dryRunRet.output.find("Credential or Endpoint Error") != std::string::npos)
            {
                Engine::pushPluginDiagnosticEvent(
                    IPluginDiagnosticEvent::Level::error, "Plugin Credential or Endpoint Error",
                    "Error with cloud credentials or incorrect endpoint: " + parseCloudfuseError(dryRunRet.output));
                NX_PRINT << "Error with cloud credentials or incorrect endpoint: " +
                                parseCloudfuseError(dryRunRet.output);
                return error(ErrorCode::otherError, "Error with cloud credentials or incorrect endpoint: " +
                                                        parseCloudfuseError(dryRunRet.output));
            }
            if (dryRunRet.output.find("Endpoint Error") != std::string::npos)
            {
                Engine::pushPluginDiagnosticEvent(IPluginDiagnosticEvent::Level::error, "Plugin Endpoint Error",
                                                  "Error with provided endpoint: " +
                                                      parseCloudfuseError(dryRunRet.output));
                NX_PRINT << "Error with provided endpoint: " + parseCloudfuseError(dryRunRet.output);
                return error(ErrorCode::otherError,
                             "Error with provided endpoint: " + parseCloudfuseError(dryRunRet.output));
            }
            if (dryRunRet.output.find("Secret Error") != std::string::npos)
            {
                Engine::pushPluginDiagnosticEvent(IPluginDiagnosticEvent::Level::error, "Plugin Secret Error",
                                                  "Secret key provided is incorrect: " +
                                                      parseCloudfuseError(dryRunRet.output));
                NX_PRINT << "Error with provided endpoint: " + parseCloudfuseError(dryRunRet.output);
                return error(ErrorCode::otherError,
                             "Secret key provided is incorrect: " + parseCloudfuseError(dryRunRet.output));
            }

            // Otherwise this is an error we did not prepare for
            Engine::pushPluginDiagnosticEvent(IPluginDiagnosticEvent::Level::error, "Plugin Error",
                                              "Unable to validate credentials with error: " +
                                                  parseCloudfuseError(dryRunRet.output));
            NX_PRINT << "Unable to validate credentials with error: " + parseCloudfuseError(dryRunRet.output);
            return error(ErrorCode::otherError,
                         "Unable to validate credentials with error: " + parseCloudfuseError(dryRunRet.output));
        }
        NX_PRINT << "spawning process from mount";
#if defined(__linux__)
        const processReturn mountRet = cfManager.mount(keyId, secretKey, passphrase);
#elif defined(_WIN32)
        const processReturn mountRet = cfManager.mount(passphrase);
#endif

        if (mountRet.errCode != 0)
        {
            NX_PRINT << "Unable to launch mount with error: " + mountRet.output;
            return error(ErrorCode::internalError, "Unable to launch mount with error: " + mountRet.output);
        }

        // Mount might not show up immediately, so wait for mount to appear
        int retryCount = 0;
        while (!cfManager.isMounted() && retryCount < 10)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            retryCount++;
        }

        if (retryCount == 10)
        {
            NX_PRINT << "Cloudfuse was not able to successfully mount";
            return error(ErrorCode::internalError, "Cloudfuse was not able to successfully mount");
        }
    }
    else
    {
        NX_PRINT << "Settings have not changed.";
    }

    if (!processActiveSettings(&model, &values))
    {
        NX_PRINT << "Unable to process the active settings section";
        return error(ErrorCode::internalError, "Unable to process the active settings section");
    }

    auto settingsResponse = new SettingsResponse();
    settingsResponse->setModel(makePtr<String>(Json(model).dump()));
    settingsResponse->setValues(makePtr<StringMap>(values));

    return settingsResponse;
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
        NX_PRINT << "Unable to process the active settings section";
        *outResult = error(ErrorCode::internalError, "Unable to process the active settings section");

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
