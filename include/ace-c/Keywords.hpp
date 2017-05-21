#ifndef KEYWORDS_HPP
#define KEYWORDS_HPP

#include <string>
#include <map>

enum Keywords {
    Keyword_module,
    Keyword_import,
    Keyword_use,
    Keyword_let,
    Keyword_const,
    Keyword_ref,
    Keyword_val,
    Keyword_func,
    Keyword_type,
    Keyword_as,
    Keyword_has,
    Keyword_new,
    Keyword_print,
    Keyword_self,
    Keyword_if,
    Keyword_else,
    Keyword_for,
    Keyword_each,
    Keyword_while,
    Keyword_do,
    Keyword_try,
    Keyword_catch,
    Keyword_throw,
    Keyword_nil,
    Keyword_void,
    Keyword_true,
    Keyword_false,
    Keyword_return,
    Keyword_break,
    Keyword_continue,
    Keyword_async,
    Keyword_pure,
    Keyword_valueof,
    Keyword_typeof
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
