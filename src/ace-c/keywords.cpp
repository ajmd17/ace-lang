#include <ace-c/keywords.hpp>

#include <stdexcept>

const std::map<std::string, Keywords> Keyword::keyword_strings = {
    { "module",   Keyword_module },
    { "import",   Keyword_import },
    { "use",      Keyword_use },
    { "let",      Keyword_let },
    { "const",    Keyword_const },
    { "ref",      Keyword_ref },
    { "val",      Keyword_val },
    { "func",     Keyword_func },
    { "type",     Keyword_type },
    { "as",       Keyword_as },
    { "print",    Keyword_print },
    { "if",       Keyword_if },
    { "else",     Keyword_else },
    { "for",      Keyword_for },
    { "while",    Keyword_while },
    { "do",       Keyword_do },
    { "try",      Keyword_try },
    { "catch",    Keyword_catch },
    { "throw",    Keyword_throw },
    { "null",     Keyword_null },
    { "void",     Keyword_void },
    { "true",     Keyword_true },
    { "false",    Keyword_false },
    { "return",   Keyword_return },
    { "break",    Keyword_break },
    { "continue", Keyword_continue },
};

bool Keyword::IsKeyword(const std::string &str)
{
    return keyword_strings.find(str) != keyword_strings.end();
}

std::string Keyword::ToString(Keywords keyword)
{
    for (auto &it : keyword_strings) {
        if (it.second == keyword) {
            return it.first;
        }
    }
    return "";
}
