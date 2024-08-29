// Copyright © 2024 Seagate Technology LLC and/or its Affiliates
// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/sdk/analytics/helpers/consuming_device_agent.h>

#include <nx/kit/json.h>

#include "engine.h"
#include "stub_analytics_plugin_settings_ini.h"

namespace settings
{

class DeviceAgent : public nx::sdk::analytics::ConsumingDeviceAgent
{
  public:
    DeviceAgent(Engine *engine, const nx::sdk::IDeviceInfo *deviceInfo);
    virtual ~DeviceAgent() override;

  protected:
    virtual void getPluginSideSettings(nx::sdk::Result<const nx::sdk::ISettingsResponse *> *outResult) const override;

    virtual void doSetNeededMetadataTypes(nx::sdk::Result<void> *outValue,
                                          const nx::sdk::analytics::IMetadataTypes *neededMetadataTypes) override;

    virtual std::string manifestString() const override;

    virtual nx::sdk::Result<const nx::sdk::ISettingsResponse *> settingsReceived() override;

    virtual void doGetSettingsOnActiveSettingChange(
        nx::sdk::Result<const nx::sdk::IActiveSettingChangedResponse *> *outResult,
        const nx::sdk::IActiveSettingChangedAction *activeSettingChangedAction) override;

  private:
    void dumpStringMap(const char *prefix, const char *appendix, const nx::sdk::IStringMap *stringMap) const;

    void dumpActiveSettingChangedAction(const nx::sdk::IActiveSettingChangedAction *activeSettingChangedAction) const;

  private:
    void processActiveSettings(nx::kit::Json *inOutSettingModel, std::map<std::string, std::string> *inOutSettingValue);

  private:
    Engine *const m_engine;
};

} // namespace settings
