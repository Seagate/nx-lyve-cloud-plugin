// Copyright © 2024 Seagate Technology LLC and/or its Affiliates
// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/kit/ini_config.h>
#include <nx/sdk/analytics/helpers/pixel_format.h>

namespace settings
{

struct Ini : public nx::kit::IniConfig
{
    Ini() : IniConfig("stub_analytics_plugin_settings.ini")
    {
        reload();
    }

    NX_INI_FLAG(0, enableOutput, "");

    NX_INI_FLAG(0, deviceDependent, "Respective capability in the manifest.");
};

Ini &ini();

} // namespace settings
