#include <athens/token.h>

Token::Token(const Token &other)
    : m_type(other.m_type),
      m_value(other.m_value),
      m_location(other.m_location)
{
}