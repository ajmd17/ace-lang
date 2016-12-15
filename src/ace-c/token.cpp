#include <ace-c/token.hpp>

const Token Token::EMPTY = Token(Token::TokenType::Token_empty, "", SourceLocation::eof);

std::string Token::TokenTypeToString(TokenType type)
{
    switch (type) {
    case Token_integer_literal:     return "integer";
    case Token_float_literal:       return "float";
    case Token_string_literal:      return "string";
    case Token_identifier:          return "identifier";
    case Token_keyword:             return "keyword";
    case Token_operator:            return "operator";
    case Token_newline:             return "newline";
    case Token_comma:               return ",";
    case Token_semicolon:           return ";";
    case Token_colon:               return ":";
    case Token_dot:                 return ".";
    case Token_ellipsis:            return "...";
    case Token_right_arrow:         return "->";
    case Token_open_parenthesis:    return "(";
    case Token_close_parenthesis:   return ")";
    case Token_open_bracket:        return "[";
    case Token_close_bracket:       return "]";
    case Token_open_brace:          return "{";
    case Token_close_brace:         return "}";
    case Token_preprocessor_symbol: return "#";
    case Token_documentation:       return "doc-comment";
    default:                        return "??";
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

bool Token::IsContinuationToken() const
{
    return m_type == Token_operator ||
        m_type == Token_comma ||
        m_type == Token_colon ||
        m_type == Token_dot ||
        m_type == Token_right_arrow ||
        m_type == Token_open_parenthesis ||
        m_type == Token_open_bracket ||
        m_type == Token_open_brace;
}
