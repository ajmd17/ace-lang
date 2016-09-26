#include "module.h"

Module::Module(const std::string &name, const SourceLocation &location)
    : m_name(name),
      m_location(location)
{
    // add the global scope
    m_scopes.push_back(Scope());
    m_current_scope = &m_scopes.back();
}

Module::Module(const Module &other)
    : m_name(other.m_name),
      m_location(other.m_location),
      m_scopes(other.m_scopes)
{
}

Scope *Module::OpenScope()
{
    /*m_current_scope->m_inner_scopes.push_back(Scope());
    Scope *new_scope = &m_current_scope->m_inner_scopes.back();
    new_scope->m_parent = m_current_scope;
    m_current_scope = new_scope;
    return m_current_scope;*/
    return nullptr;
}

Scope *Module::CloseScope()
{
   // Scope *parent_scope = m_current_scope->m_parent;
    return nullptr;
}