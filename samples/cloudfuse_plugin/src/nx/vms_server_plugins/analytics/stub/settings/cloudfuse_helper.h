#include <string>

// parseCloudfuseError takes in an error and trims the error down to it's most essential
// error, which from cloudfuse is the error returned between braces []
std::string parseCloudfuseError(std::string error);
