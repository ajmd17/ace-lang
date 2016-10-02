#include <athens/token.h>

std::string Token::TokenTypeToString(TokenType type)
{
    switch (type) {
    case Token_empty:
        return "empty";
    case Token_integer_literal:
        return "int";
    case Token_float_literal:
        return "float";
    case Token_string_literal:
        return "string";
    case Token_identifier:
        return "identifier";
    case Token_keyword:
        return "keyword";
    case Token_operator:
        return "operator";
    case Token_open_parenthesis:
        return "(";
    case Token_close_parenthesis:
        return ")";
    case Token_open_bracket:
        return "[";
    case Token_close_bracket:
        return "]";
    case Token_open_brace:
        return "{";
    case Token_close_brace:
        return "}";
    case Token_preprocessor_symbol:
        return "#";
    case Token_documentation:
        return "/**";
    }
}

Token::Token(TokenType type, const std::string &value, const SourceLocation &location)
    : m_type(type),
      m_value(value),
      m_location(location)
{
}

Token::Token(const Token &other)
    : m_type(other.m_type),
      m_value(other.m_value),
      m_location(other.m_location)
{
}