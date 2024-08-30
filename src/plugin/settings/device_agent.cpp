// Copyright © 2024 Seagate Technology LLC and/or its Affiliates
// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "device_agent.h"

#include <algorithm>

#define NX_PRINT_PREFIX (this->logUtils.printPrefix)
#include <nx/kit/debug.h>
#include <nx/kit/utils.h>
#include <nx/sdk/helpers/active_setting_changed_response.h>
#include <nx/sdk/helpers/error.h>

#include "settings_model.h"
#include "stub_analytics_plugin_settings_ini.h"

namespace settings
{

using nx::kit::Json;
using namespace nx::sdk;
using namespace nx::sdk::analytics;

DeviceAgent::DeviceAgent(Engine *engine, const nx::sdk::IDeviceInfo *deviceInfo)
    : ConsumingDeviceAgent(deviceInfo, NX_DEBUG_ENABLE_OUTPUT, engine->plugin()->instanceId()), m_engine(engine)
{
}

DeviceAgent::~DeviceAgent()
{
}

std::string DeviceAgent::manifestString() const
{
    return /*suppress newline*/ 1 + (const char *)R"json(
{
    "capabilities": "disableStreamSelection"
}
)json";
}

Result<const ISettingsResponse *> DeviceAgent::settingsReceived()
{
    return Error(ErrorCode::noError, nullptr);
}

void DeviceAgent::doGetSettingsOnActiveSettingChange(Result<const IActiveSettingChangedResponse *> *outResult,
                                                     const IActiveSettingChangedAction *activeSettingChangedAction)
{
}

void DeviceAgent::processActiveSettings(nx::kit::Json *inOutSettingsModel,
                                        std::map<std::string, std::string> *inOutSettingsValues)
{
}

void DeviceAgent::doSetNeededMetadataTypes(Result<void> * /*outResult*/, const IMetadataTypes * /*neededMetadataTypes*/)
{
}

void DeviceAgent::getPluginSideSettings(Result<const ISettingsResponse *> *outResult) const
{
}

} // namespace settings
