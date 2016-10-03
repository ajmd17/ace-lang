#ifndef KEYWORDS_H
#define KEYWORDS_H

#include <string>
#include <map>

enum Keywords {
    Keyword_module,
    Keyword_import,
    Keyword_use,
    Keyword_var,
    Keyword_const,
    Keyword_alias,
    Keyword_func,
    Keyword_print,
    Keyword_if,
    Keyword_else,
    Keyword_for,
    Keyword_while,
    Keyword_do,
    Keyword_try,
    Keyword_catch,
    Keyword_throw,
    Keyword_null,
    Keyword_void,
    Keyword_true,
    Keyword_false,
    Keyword_return,
    Keyword_break,
    Keyword_continue,
};

class Keyword {
/* Static class members */
public:
    static bool IsKeyword(const std::string &str);
    static std::string ToString(Keywords keyword);

private:
    static const std::map<std::string, Keywords> keyword_strings;
};

#endif