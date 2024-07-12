// Copyright © 2024 Seagate Technology LLC and/or its Affiliates
// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include <nx/kit/debug.h>

#include "settings/plugin.h"

extern "C" NX_PLUGIN_API nx::sdk::IPlugin *createNxPlugin()
{
    return new settings::Plugin();
}
