// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "actions.h"

#include <nx/kit/debug.h>
#include <nx/kit/utils.h>

#include "settings_model.h"

namespace nx
{
namespace vms_server_plugins
{
namespace analytics
{
namespace stub
{
namespace settings
{

void dumpStringMap(const char *prefix, const char *appendix, const nx::sdk::IStringMap *stringMap);

nx::sdk::Ptr<nx::sdk::ActionResponse> generateActionResponse(const std::string &settingId,
                                                             nx::sdk::Ptr<const nx::sdk::IStringMap> params)
{
    using namespace nx::sdk;
    NX_PRINT << "cloudfuse actions.cpp:generateActionResponse(" << settingId << ")" << std::endl;
    dumpStringMap("cloudfuse", NULL, params.get());

    if (settingId == kCheckCredentialsButtonId)
    {
        // run Cloudfuse --dry-run to check credentials
        // hard-coding a response for now
        const char *cloudfuseOutput = "Cloudfuse call not implemented!";
        bool success = false;
        // possible responses to user
        std::string successMessage = "✅ Credentials verified";
        std::string failureMessage = "❌ Cloud connection or authentication error";
        std::string resultMessage;

        // build action response
        const auto actionResponse = makePtr<ActionResponse>();
        if (success)
        {
            resultMessage = successMessage;
        }
        else
        {
            resultMessage = failureMessage;
        }
        actionResponse->setMessageToUser(resultMessage +
                                         " \nCheck Output: " + nx::kit::utils::toString(cloudfuseOutput));

        return actionResponse;
    }

    return nullptr;
}

void dumpStringMap(const char *prefix, const char *appendix, const nx::sdk::IStringMap *stringMap)
{
    if (!stringMap)
    {
        NX_PRINT << prefix << "    null" << appendix;
        return;
    }

    NX_PRINT << prefix << "{";
    for (int i = 0; i < stringMap->count(); ++i)
    {
        NX_PRINT << prefix << "    " << nx::kit::utils::toString(stringMap->key(i)) << ": "
                 << nx::kit::utils::toString(stringMap->value(i)) << ((i == stringMap->count() - 1) ? "" : ",");
    }
    NX_PRINT << prefix << "}" << appendix;
}

} // namespace settings
} // namespace stub
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
