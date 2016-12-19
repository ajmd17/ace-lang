#ifndef STR_UTIL_HPP
#define STR_UTIL_HPP

#include <string>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

namespace str_util {

inline std::string strip_extension(const std::string &filename) {
    auto pos = filename.find_last_of(".");
    if (pos == std::string::npos) {
        return filename;
    }
    return filename.substr(0, pos);
}

inline std::string left_trim(std::string s) {
    auto predicate = [](char c) -> bool {
        return !std::isspace(c);
    };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), predicate));
    return s;
}

inline std::string right_trim(std::string s) {
    auto predicate = [](char c) -> bool {
        return !std::isspace(c);
    };
    s.erase(std::find_if(s.rbegin(), s.rend(), predicate).base(), s.end());
    return s;
}

inline std::string trim(std::string s) {
    return left_trim(right_trim(s));
}

} // str_util

#endif
