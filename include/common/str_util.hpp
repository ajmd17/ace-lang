#ifndef STR_UTIL_HPP
#define STR_UTIL_HPP

#include <string>

namespace str_util {
inline std::string strip_extension(const std::string &filename)
{
    auto pos = filename.find_last_of(".");
    if (pos == std::string::npos) {
        return filename;
    }
    return filename.substr(0, pos);
}
} // str_util

#endif
