#include <ace-c/scope.hpp>

Scope::Scope()
    : m_scope_type(SCOPE_TYPE_NORMAL)
{
}

Scope::Scope(ScopeType scope_type)
    : m_scope_type(scope_type)
{
}

Scope::Scope(const Scope &other)
    : m_identifier_table(other.m_identifier_table),
      m_scope_type(other.m_scope_type),
      m_return_types(other.m_return_types)
{
}
