// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <string>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace stub {
namespace settings {

static const std::string kCaption = "caption";
static const std::string kSections = "sections";
static const std::string kName = "name";
static const std::string kRange = "range";
static const std::string kItems = "items";
static const std::string kIsActive = "isActive";
static const std::string kMinValue = "minValue";
static const std::string kMaxValue = "maxValue";

// ------------------------------------------------------------------------------------------------
// TODO: rip out these old stub settings model variables
static const std::string kRegularSettingsModelOption = "regular";
static const std::string kAlternativeSettingsModelOption = "alternative";
static const std::string kSettingsModelSettings = "settingsModelComboBox";
static const std::string kCitySelector = "languageSelectorSettings";
static const std::string kEnglishOption = "English";
static const std::string kGermanOption = "German";
static const std::string kActiveSettingsSectionCaption = "Active settings section";
static const std::string kAdvancedSettingsGroupBoxCaption = "Advanced Settings";
static const std::string kActiveComboBoxId = "activeComboBox";
static const std::string kAdditionalComboBoxId = "additionalComboBox";
static const std::string kShowAdditionalComboBoxValue = "Show additional ComboBox";
static const std::string kAdditionalComboBoxValue = "Value 1";
static const std::string kActiveCheckBoxId = "activeCheckBox";
static const std::string kAdditionalCheckBoxId = "additionalCheckBox";
static const std::string kShowAdditionalCheckBoxValue = "true";
static const std::string kAdditionalCheckBoxValue = "false";
static const std::string kActiveRadioButtonGroupId = "activeRadioButtonGroup";
static const std::string kShowAdditionalRadioButtonValue = "Show something";
static const std::string kHideAdditionalRadioButtonValue = "Hide me";
static const std::string kDefaultActiveRadioButtonGroupValue = "Some value";
static const std::string kActiveMinValueId = "activeMinValue";
static const std::string kActiveMaxValueId = "activeMaxValue";
static const std::string kShowUrlButtonId = "showUrlButton";
static const std::string kEnginePluginSideSetting = "testPluginSideSpinBox";
static const std::string kEnginePluginSideSettingValue = "42";
static const std::string kAlternativeSettingsModel = /*suppress newline*/ 1 + (const char*) R"json(")json";
static const std::string kRegularSettingsModelPart1 = /*suppress newline*/ 1 + R"json()json";
static const std::string kEnglishCitiesSettingsModelPart = /*suppress newline*/ 1 + R"json()json";
static const std::string kGermanCitiesSettingsModelPart = /*suppress newline*/ 1 + R"json()json";
static const std::string kRegularSettingsModelPart2 = /*suppress newline*/ 1 + R"json(")json";
// ------------------------------------------------------------------------------------------------

static const std::string kKeyIdTextFieldId = "keyId";
static const std::string kSecretKeyPasswordFieldId = "secretKey";
static const std::string kCheckCredentialsButtonId = "checkCredentialsButton";
// advanced
static const std::string kChangeEndpointButtonId = "changeEndpointButton";
static const std::string kChangeConfigPassphraseButtonId = "changePassphraseButton";
// endpoint URL pop-up dialog
static const std::string kEndpointUrlTextFieldId = "endpointUrl";
static const std::string kChangeEndpointModel = /*suppress newline*/ 1 + R"json(
{
    "type": "Settings",
    "items":
    [
        {
            "type": "TextField",
            "name": ")json" + kEndpointUrlTextFieldId + R"json(",
            "caption": "Endpoint URL",
            "description": "Set a different endpoint (different region or service)",
            "defaultValue": "https://s3.us-east-1.lyvecloud.seagate.com",
            "validationErrorMessage": "Endpoint must begin with 'http[s]://'",
            "validationRegex": "^https?://.+$",
            "validationRegexFlags": "i"
        },
        {
            "type": "Banner",
            "text": "e.g. https://s3.us-east-1.lyvecloud.seagate.com"
        }
    ]
}
)json";
// config passphrase pop-up dialog
static const std::string kPassphrasePasswordFieldId = "configPassphrase";
static const std::string kChangePassphraseModel = /*suppress newline*/ 1 + R"json("
{
    "type": "Settings",
    "items":
    [
        {
            "type": "PasswordField",
            "name": ")json" + kPassphrasePasswordFieldId + R"json(",
            "caption": "Config Passphrase",
            "description": "Choose passphrase to encrypt S3 credentials in config file",
            "defaultValue": ""
        }
    ]
}
)json";

// ------------------------------------------------------------------------------------------------
static const std::string kEngineSettingsModel = /*suppress newline*/ 1 + R"json("
{
    "type": "Settings",
    "items":
    [
        {
            "type": "GroupBox",
            "caption": "Credentials",
            "items":
            [
                {
                    "type": "TextField",
                    "name": ")json" + kKeyIdTextFieldId + R"json(",
                    "caption": "Access Key ID",
                    "description": "Cloud bucket access key ID",
                    "defaultValue": "",
                    "validationErrorMessage": "Access key ID must be 20 alphanumeric characters",
                    "validationRegex": "^[A-Z2-7]{16,2048}$",
                    "validationRegexFlags": "i",
                    "isActive": true
                },
                {
                    "type": "PasswordField",
                    "name": ")json" + kSecretKeyPasswordFieldId + R"json(",
                    "caption": "Secret Key",
                    "description": "Cloud bucket secret key",
                    "defaultValue": "",
                    "validationErrorMessage": "Secret key must be 40 alphanumeric-plus-slash characters",
                    "validationRegex": "^[A-Z0-9\\+\\\/]{40}$",
                    "validationRegexFlags": "i",
                    "isActive": true
                }
            ]
        },
        {
            "type": "GroupBox",
            "caption": ")json" + kAdvancedSettingsGroupBoxCaption + R"json(",
            "items":
            [
                {
                    "type": "Button",
                    "name": ")json" + kChangeEndpointButtonId + R"json(",
                    "caption": "Change S3 Endpoint...",
                    "isActive": true,
                    "parametersModel": )json" + kChangeEndpointModel + R"json(
                },
                {
                    "type": "Button",
                    "name": ")json" + kChangeConfigPassphraseButtonId + R"json(",
                    "caption": "Change Config Passphrase...",
                    "isActive": true,
                    "parametersModel": )json" + kChangePassphraseModel + R"json(
                }
            ]
        },
        {
            "type": "Link",
            "caption": "Plugin Website",
            "url": "https://github.com/Seagate/nx-lyve-cloud-plugin"
        }
    ]
}
)json";

} // namespace settings
} // namespace stub
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
