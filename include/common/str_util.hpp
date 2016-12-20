#ifndef STR_UTIL_HPP
#define STR_UTIL_HPP

#include <string>
#include <vector>
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

inline std::vector<std::string> split_path(const std::string &str) {
    std::vector<std::string> res;
    
    std::string tmp;
    for (char ch : str) {
        if (ch == '\\' || ch == '/') {
            if (!tmp.empty()) {
                res.emplace_back(tmp);
                tmp.clear();
            }
            continue;
        }

        tmp += ch;
    }

    // add last
    if (!tmp.empty()) {
        res.emplace_back(tmp);
    }

    return res;
}

inline std::vector<std::string> canonicalize_path(const std::vector<std::string> &original) {
    std::vector<std::string> res;

    for (const auto &str : original) {
        if (str == ".." && !res.empty()) {
            res.pop_back();
        } else if (str != ".") {
            res.emplace_back(str);
        }
    }

    return res;
}

inline std::string path_to_str(const std::vector<std::string> &path) {
    std::string res;
    
    for (size_t i = 0; i < path.size(); i++) {
        res += path[i];
        if (i != path.size() - 1) {
            res += "/";
        }
    }

    return res;
}

} // str_util

#endif
