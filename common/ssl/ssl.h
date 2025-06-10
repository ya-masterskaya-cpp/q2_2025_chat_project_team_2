#include <string>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

namespace ssl{

std::string HashPassword(const std::string& password);

}//end namespace ssl