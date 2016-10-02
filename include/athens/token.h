#ifndef TOKEN_H
#define TOKEN_H

#include <athens/source_location.h>

#include <string>

enum TokenType {
    Token_empty,
    Token_integer_literal,
    Token_float_literal,
    Token_string_literal,
    Token_identifier,
    Token_keyword,
    Token_operator,
    Token_open_parenthesis,
    Token_close_parenthesis,
    Token_open_bracket,
    Token_close_bracket,
    Token_open_brace,
    Token_close_brace,
    Token_preprocessor_symbol,
    Token_documentation
};

class Token {
public:
    static std::string TokenTypeToString(TokenType type);

public:
    Token(TokenType type, const std::string &value, 
        const SourceLocation &location);
    Token(const Token &other);

    inline TokenType GetType() const { return m_type; }
    inline const std::string &GetValue() const { return m_value; }
    inline const SourceLocation &GetLocation() const { return m_location; }
    inline bool Empty() const { return m_type == Token_empty; }

private:
    TokenType m_type;
    std::string m_value;
    SourceLocation m_location;
};

#endif