// Copyright © 2024 Seagate Technology LLC and/or its Affiliates
// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include "active_settings_builder.h"

#include <nx/sdk/i_string.h>
#include <nx/sdk/i_string_map.h>

namespace settings
{

extern const std::map<ActiveSettingsBuilder::ActiveSettingKey, ActiveSettingsBuilder::ActiveSettingHandler>
    kActiveSettingsRules;

extern const std::map<
    /*activeSettingName*/ std::string, ActiveSettingsBuilder::ActiveSettingHandler>
    kDefaultActiveSettingsRules;

void showAdditionalComboBox(nx::kit::Json *inOutModel, std::map<std::string, std::string> *inOutValues);

void hideAdditionalComboBox(nx::kit::Json *inOutModel, std::map<std::string, std::string> *inOutValues);

void showAdditionalCheckBox(nx::kit::Json *inOutModel, std::map<std::string, std::string> *inOutValues);

void hideAdditionalCheckBox(nx::kit::Json *inOutModel, std::map<std::string, std::string> *inOutValues);

void showAdditionalRadioButton(nx::kit::Json *inOutModel, std::map<std::string, std::string> *inOutValues);

void hideAdditionalRadioButton(nx::kit::Json *inOutModel, std::map<std::string, std::string> *inOutValues);

void updateMinMaxSpinBoxes(nx::kit::Json *inOutModel, std::map<std::string, std::string> *inOutValues);

} // namespace settings