#include "cloudfuse_helper.h"

// parseCloudfuseError takes in an error and trims the error down to it's most essential
// error, which from cloudfuse is the error returned between braces []
std::string parseCloudfuseError(std::string error)
{
    std::size_t start = error.find_last_of('[');
    if (start == std::string::npos)
    {
        return error;
    }

    std::size_t end = error.find_last_of(']');
    if (end == std::string::npos)
    {
        return error;
    }

    return error.substr(start, end);
}
