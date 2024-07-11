// Copyright © 2024 Seagate Technology LLC and/or its Affiliates
// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/sdk/analytics/helpers/plugin.h>
#include <nx/sdk/analytics/i_engine.h>

namespace settings
{

class Plugin : public nx::sdk::analytics::Plugin
{
  public:
    Plugin() = default;

  protected:
    virtual nx::sdk::Result<nx::sdk::analytics::IEngine *> doObtainEngine() override;
    virtual std::string instanceId() const override
    {
        return "seagate.cloudfuse";
    }
    virtual std::string manifestString() const override;
};

} // namespace settings
