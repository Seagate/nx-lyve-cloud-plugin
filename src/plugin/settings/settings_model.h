// Copyright © 2024 Seagate Technology LLC and/or its Affiliates
// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <cstdint>
#include <string>

namespace settings
{

static const std::string kName = "name";
static const std::string kItems = "items";

// subscription
static const std::string kSubscriptionKeyFieldId = "subscriptionKey";
static const std::string kCheckCredentialsButtonId = "checkCredentialsButton";

// top-level settings model
static const std::string kEngineSettingsModel = /*suppress newline*/ 1 + R"json(
{
    "type": "Settings",
    "items":
    [
        {
            "type": "TextField",
            "name": ")json" + kSubscriptionKeyFieldId +
                                                R"json(",
            "caption": "Subscription Key",
            "description": "DW Cumulus Subscription Key",
            "defaultValue": ""
        }
    ]
}
)json";

// status
static const std::string kBucketStatusBannerId = "connectionStatus";
static const std::string kSubscriptionStatusBannerId = "subscriptionStatus";
static const std::string kStatusSuccess = R"json(
        {
            "type": "Banner",
            "name": ")json" + kBucketStatusBannerId +
                                          R"json(",
            "icon": "info",
            "text": "Cloud storage connected successfully!"
        }
)json";
static const std::string kStatusFailure = R"json(
        {
            "type": "Banner",
            "name": ")json" + kBucketStatusBannerId +
                                          R"json(",
            "icon": "warning",
            "text": "Cloud storage connection failed!"
        }
)json";
static const std::string kStatusSaaSSubscriptionVerified = R"json(
        {
            "type": "Banner",
            "name": ")json" + kSubscriptionStatusBannerId +
                                                           R"json(",
            "icon": "info",
            "text": "Plugin authorized - SaaS subscription verified"
        }
)json";
static const std::string kStatusNoSaaSSubscription = R"json(
        {
            "type": "Banner",
            "name": ")json" + kSubscriptionStatusBannerId +
                                                     R"json(",
            "icon": "warning",
            "text": "Plugin unauthorized - SaaS subscription required"
        }
)json";
static const std::string kStatusUnkownSaaSSubscription = R"json(
        {
            "type": "Banner",
            "name": ")json" + kSubscriptionStatusBannerId +
                                                         R"json(",
            "icon": "info",
            "text": "SaaS subscription status: Pending verification"
        }
)json";

} // namespace settings
