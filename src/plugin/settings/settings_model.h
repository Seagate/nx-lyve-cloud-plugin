// Copyright © 2024 Seagate Technology LLC and/or its Affiliates
// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <cstdint>
#include <string>

namespace settings
{

static const std::string kName = "name";
static const std::string kItems = "items";

// Enable this flag hide all but the credentials section
// NOTE: enabling this will prevent the user from changing the default endpoint (kDefaultEndpoint)
// Only set this flag true if you want to tie your users to a specific cloud storage endpoint
static const bool credentialsOnly = false;

// credentials
static const std::string kKeyIdTextFieldId = "keyId";
static const std::string kSecretKeyPasswordFieldId = "secretKey";
static const std::string kCheckCredentialsButtonId = "checkCredentialsButton";
static const std::string kCredentialGroupBox = R"json(
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
                    "defaultValue": ""
                },
                {
                    "type": "PasswordField",
                    "name": ")json" + kSecretKeyPasswordFieldId +
                                               R"json(",
                    "caption": "Secret Key",
                    "description": "Cloud bucket secret key",
                    "defaultValue": ""
                }
            ]
        })json";

// advanced
static const std::string kEndpointUrlTextFieldId = "endpointUrl";
static const std::string kDefaultEndpoint = "https://s3.us-east-1.lyvecloud.seagate.com";
static const std::string kBucketNameTextFieldId = "bucketName";
static const std::string kBucketSizeTextFieldId = "bucketCapacity";
static const uint64_t kDefaultBucketSizeGb = 1024;
static const std::string kAdvancedGroupBox = R"json(
        {
            "type": "GroupBox",
            "caption": "Advanced Settings",
            "items":
            [
                {
                    "type": "TextField",
                    "name": ")json" + kEndpointUrlTextFieldId +
                                             R"json(",
                    "caption": "Endpoint URL",
                    "description": "Set a different endpoint (different region or service)",
                    "defaultValue": ")json" + kDefaultEndpoint +
                                             R"json("
                },
                {
                    "type": "TextField",
                    "name": ")json" + kBucketNameTextFieldId +
                                             R"json(",
                    "caption": "Bucket Name",
                    "description": "Specify a bucket name (leave empty to let the system automatically detect your bucket)",
                    "defaultValue": ""
                },
                {
                    "type": "SpinBox",
                    "name": ")json" + kBucketSizeTextFieldId +
                                             R"json(",
                    "caption": "Backup Storage Limit (in GB)",
                    "description": "Maximum data this server should back up - default is )json" +
                                             std::to_string(kDefaultBucketSizeGb) +
                                             R"json(GB",
                    "defaultValue": )json" + std::to_string(kDefaultBucketSizeGb) +
                                             R"json(,
                    "minValue": 1,
                    "maxValue": 1000000000
                }
            ]
        })json";

static const std::string kPluginWebsiteLink = R"json(
        {
            "type": "Link",
            "caption": "Plugin Website",
            "url": "https://github.com/Seagate/nx-lyve-cloud-plugin"
        })json";

// gather settings items together
static const std::string kSettingsItems =
    kCredentialGroupBox + (credentialsOnly ? "" : ("," + kAdvancedGroupBox + "," + kPluginWebsiteLink));

// top-level settings model
static const std::string kEngineSettingsModel = /*suppress newline*/ 1 + R"json(
{
    "type": "Settings",
    "items":
    [)json" + kSettingsItems + R"json(
    ]
}
)json";

// status
static const std::string kStatusBannerId = "connectionStatus";
static const std::string kStatusSuccess = R"json(
        {
            "type": "Banner",
            "name": ")json" + kStatusBannerId +
                                          R"json(",
            "icon": "info",
            "text": "Cloud storage connected successfully!"
        }
)json";
static const std::string kStatusFailure = R"json(
        {
            "type": "Banner",
            "name": ")json" + kStatusBannerId +
                                          R"json(",
            "icon": "warning",
            "text": "Cloud storage connection failed!"
        }
)json";
static const std::string kStatusNoSaaSSubscription = R"json(
        {
            "type": "Banner",
            "name": ")json" + kStatusBannerId +
                                          R"json(",
            "icon": "warning",
            "text": "Plugin unauthorized - SaaS subscription required"
        }
)json";

} // namespace settings
