#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <ace-c/source_location.hpp>

#include <string>

class Token {
public:
    enum TokenType {
      Token_empty,
      Token_integer_literal,
      Token_float_literal,
      Token_string_literal,
      Token_identifier,
      Token_keyword,
      Token_operator,
      Token_comma,
      Token_semicolon,
      Token_colon,
      Token_dot,
      Token_ellipsis,
      Token_right_arrow,
      Token_open_parenthesis,
      Token_close_parenthesis,
      Token_open_bracket,
      Token_close_bracket,
      Token_open_brace,
      Token_close_brace,
      Token_preprocessor_symbol,
      Token_documentation
    };

    static std::string TokenTypeToString(Token::TokenType type);

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
