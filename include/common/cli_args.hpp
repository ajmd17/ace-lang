#ifndef CLI_ARGS_HPP
#define CLI_ARGS_HPP

#include <algorithm>
#include <string>

class CLI {
public:
    /** check if the option is set */
    static inline bool HasOption(char **begin, char **end, const std::string &opt)
    {
        return std::find(begin, end, opt) != end;
    }

    /** retrieve the value that is found after an option */
    static inline char *GetOptionValue(char **begin, char **end, const std::string &opt)
    {
        char **it = std::find(begin, end, opt);
        if (it != end && ++it != end) {
            return *it;
        }
        return nullptr;
    }
};

#endif
