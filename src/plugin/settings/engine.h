// Copyright © 2024 Seagate Technology LLC and/or its Affiliates
// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/sdk/analytics/helpers/engine.h>
#include <nx/sdk/analytics/helpers/plugin.h>
#include <nx/kit/json.h>

#include <cloudfuse/child_process.h>

namespace settings
{

class Engine : public nx::sdk::analytics::Engine
{
  public:
    Engine(nx::sdk::analytics::Plugin *plugin);
    virtual ~Engine() override;

    nx::sdk::analytics::Plugin *const plugin() const
    {
        return m_plugin;
    }

  protected:
    virtual std::string manifestString() const override;

    virtual nx::sdk::Result<const nx::sdk::ISettingsResponse *> settingsReceived() override;
    bool mount();

  protected:
    virtual void doObtainDeviceAgent(nx::sdk::Result<nx::sdk::analytics::IDeviceAgent *> *outResult,
                                     const nx::sdk::IDeviceInfo *deviceInfo) override;

    virtual void getPluginSideSettings(nx::sdk::Result<const nx::sdk::ISettingsResponse *> *outResult) const override;

    virtual void doGetSettingsOnActiveSettingChange(
        nx::sdk::Result<const nx::sdk::IActiveSettingChangedResponse *> *outResult,
        const nx::sdk::IActiveSettingChangedAction *activeSettingChangedAction) override;

  private:
    bool settingsChanged();
    nx::sdk::Error validateMount();
    nx::sdk::Error spawnMount();
    bool updateModel(nx::kit::detail::json11::Json::object *model, bool mountSuccessful) const;

  private:
    nx::sdk::analytics::Plugin *const m_plugin;
    CloudfuseMngr m_cfManager;
    std::map<std::string, std::string> m_prevSettings;
    std::string m_passphrase;
};

} // namespace settings
