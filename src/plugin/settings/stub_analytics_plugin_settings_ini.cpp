// Copyright © 2024 Seagate Technology LLC and/or its Affiliates
// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "stub_analytics_plugin_settings_ini.h"

namespace settings
{

Ini &ini()
{
    static Ini ini;
    return ini;
}

} // namespace settings
