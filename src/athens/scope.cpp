#include <athens/scope.hpp>

Scope::Scope()
{
}

Scope::Scope(const Scope &other)
    : m_identifier_table(other.m_identifier_table)
{
}
