#include <athens/scope.h>

Scope::Scope()
    : m_parent(nullptr)
{
}

Scope::Scope(const Scope &other)
    : m_identifier_table(other.m_identifier_table),
      m_parent(other.m_parent),
      m_inner_scopes(other.m_inner_scopes)
{
}
