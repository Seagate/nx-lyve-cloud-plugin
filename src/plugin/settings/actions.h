// Copyright © 2024 Seagate Technology LLC and/or its Affiliates
// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <string>

#include <nx/sdk/helpers/action_response.h>
#include <nx/sdk/i_string_map.h>
#include <nx/sdk/ptr.h>

namespace settings
{

nx::sdk::Ptr<nx::sdk::ActionResponse> generateActionResponse(const std::string &settingId,
                                                             nx::sdk::Ptr<const nx::sdk::IStringMap> params);

} // namespace settings
