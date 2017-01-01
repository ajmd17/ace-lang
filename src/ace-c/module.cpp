#include <ace-c/Module.hpp>
#include <ace-c/Configuration.hpp>

#include <common/my_assert.hpp>

Module::Module(const std::string &name, const SourceLocation &location)
    : m_name(name),
      m_location(location),
      m_tree_link(nullptr)
{
}

Identifier *Module::LookUpIdentifier(const std::string &name, bool this_scope_only)
{
    TreeNode<Scope> *top = m_scopes.TopNode();

    while (top) {
        if (Identifier *result = top->m_value.GetIdentifierTable().LookUpIdentifier(name)) {
            // a result was found
            return result;
        }

        if (this_scope_only) {
            return nullptr;
        }

        top = top->m_parent;
    }

    if (m_tree_link && m_tree_link->m_parent) {
        if (Module *other = m_tree_link->m_parent->m_value) {
            if (other->GetLocation().GetFileName() == m_location.GetFileName()) {
                return other->LookUpIdentifier(name, false);
            } else {
                // we are outside of file scope, so loop until root/global module found
                auto *link = m_tree_link->m_parent;

                while (link->m_parent) {
                    link = link->m_parent;
                }

                ASSERT(link->m_value != nullptr);
                ASSERT(link->m_value->GetName() == ace::compiler::Config::GLOBAL_MODULE_NAME);

                return link->m_value->LookUpIdentifier(name, false);
            }
        }
    }

    return nullptr;
}

Identifier *Module::LookUpIdentifierDepth(const std::string &name, int depth_level)
{
    TreeNode<Scope> *top = m_scopes.TopNode();

    for (int i = 0; top != nullptr && i < depth_level; i++) {
        Identifier *result = top->m_value.GetIdentifierTable().LookUpIdentifier(name);

        if (result) {
            return result;
        }

        top = top->m_parent;
    }

    return nullptr;
}

SymbolTypePtr_t Module::LookupSymbolType(const std::string &name)
{
    TreeNode<Scope> *top = m_scopes.TopNode();

    while (top) {
        if (SymbolTypePtr_t result = top->m_value.GetIdentifierTable().LookupSymbolType(name)) {
            // a result was found
            return result;
        }

        top = top->m_parent;
    }

    if (m_tree_link && m_tree_link->m_parent) {
        if (Module *other = m_tree_link->m_parent->m_value) {
            if (other->GetLocation().GetFileName() == m_location.GetFileName()) {
                return other->LookupSymbolType(name);
            } else {
                // we are outside of file scope, so loop until root/global module found
                auto *link = m_tree_link->m_parent;

                while (link->m_parent) {
                    link = link->m_parent;
                }

                ASSERT(link->m_value != nullptr);
                ASSERT(link->m_value->GetName() == ace::compiler::Config::GLOBAL_MODULE_NAME);

                return link->m_value->LookupSymbolType(name);
            }
        }
    }

    return nullptr;
}