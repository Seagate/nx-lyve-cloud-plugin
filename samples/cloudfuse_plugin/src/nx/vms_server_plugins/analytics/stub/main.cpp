// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include <nx/kit/debug.h>

#include "settings/plugin.h"

extern "C" NX_PLUGIN_API nx::sdk::IPlugin *createNxPlugin()
{
    using namespace nx::vms_server_plugins::analytics::stub;
    return new settings::Plugin();
}
