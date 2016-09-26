#include <athens/identifier.h>

Identifier::Identifier(const std::string &name, int index)
    : m_name(name),
      m_index(index),
      m_usecount(0),
      m_flags(0)
{
}

Identifier::Identifier(const Identifier &other)
    : m_name(other.m_name),
      m_index(other.m_index),
      m_usecount(other.m_usecount),
      m_flags(other.m_flags)
{
}