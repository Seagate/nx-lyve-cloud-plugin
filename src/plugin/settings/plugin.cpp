// Copyright © 2024 Seagate Technology LLC and/or its Affiliates
// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "plugin.h"

#include <nx/kit/debug.h>
#include <nx/kit/utils.h>

#include "engine.h"
#include "settings_model.h"

namespace settings
{

using namespace nx::sdk;
using namespace nx::sdk::analytics;

Result<IEngine *> Plugin::doObtainEngine()
{
    return new Engine(this);
}

std::string Plugin::manifestString() const
{
    return /*suppress newline*/ 1 + (const char *)R"json(
{
    "id": ")json" +
           instanceId() + R"json(",
    "name": "Lyve Cloud Backup Storage",
    "description": "Connect a cloud storage container as a backup location.",
    "version": "1.0.0",
    "vendor": "Seagate Technology",
    "engineSettingsModel": )json" +
           kEngineSettingsModel + R"json(
}
)json";
}

} // namespace settings
