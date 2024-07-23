// Copyright © 2024 Seagate Technology LLC and/or its Affiliates
// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <string>

namespace settings
{

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
static const std::string kAlternativeSettingsModel = /*suppress newline*/ 1 + (const char *)R"json(")json";
static const std::string kRegularSettingsModelPart1 = /*suppress newline*/ 1 + R"json()json";
static const std::string kEnglishCitiesSettingsModelPart = /*suppress newline*/ 1 + R"json()json";
static const std::string kGermanCitiesSettingsModelPart = /*suppress newline*/ 1 + R"json()json";
static const std::string kRegularSettingsModelPart2 = /*suppress newline*/ 1 + R"json(")json";
// ------------------------------------------------------------------------------------------------

static const std::string kKeyIdTextFieldId = "keyId";
static const std::string kSecretKeyPasswordFieldId = "secretKey";
static const std::string kCheckCredentialsButtonId = "checkCredentialsButton";
// advanced
static const std::string kEndpointUrlTextFieldId = "endpointUrl";
static const std::string kDefaultEndpoint = "https://s3.us-east-1.lyvecloud.seagate.com";
static const std::string kBucketNameTextFieldId = "bucketName";

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
                    "name": ")json" + kKeyIdTextFieldId +
                                                R"json(",
                    "caption": "Access Key ID",
                    "description": "Cloud bucket access key ID",
                    "defaultValue": "",
                    "validationErrorMessage": "Access key ID must be >=16 alphanumeric characters (uppercase or 2-7).",
                    "validationRegex": "^[A-Z1-9]{16,128}$"
                },
                {
                    "type": "PasswordField",
                    "name": ")json" + kSecretKeyPasswordFieldId +
                                                R"json(",
                    "caption": "Secret Key",
                    "description": "Cloud bucket secret key",
                    "defaultValue": "",
                    "validationErrorMessage": "Secret key must be 32 or 40 alphanumeric-plus-slash characters",
                    "validationRegex": "^[A-Za-z0-9\/+=]{32,128}$"
                }
            ]
        },
        {
            "type": "GroupBox",
            "caption": ")json" + kAdvancedSettingsGroupBoxCaption +
                                                R"json(",
            "items":
            [
                {
                    "type": "TextField",
                    "name": ")json" + kEndpointUrlTextFieldId +
                                                R"json(",
                    "caption": "Endpoint URL",
                    "description": "Set a different endpoint (different region or service)",
                    "defaultValue": ")json" + kDefaultEndpoint +
                                                R"json(",
                    "validationErrorMessage": "Endpoint must be a URL (begin with 'http[s]://').",
                    "validationRegex": "(^$)|(^https?:\/\/.+$)",
                    "validationRegexFlags": "i"
                },
                {
                    "type": "TextField",
                    "name": ")json" + kBucketNameTextFieldId +
                                                R"json(",
                    "caption": "Bucket Name",
                    "description": "Specify a bucket name (leave empty to let the system automatically detect your bucket)",
                    "defaultValue": "",
                    "validationErrorMessage": "Bucket name can only contain lowercase letters, numbers, dashes, and dots.",
                    "validationRegex": "^[-.a-z0-9]*$"
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
