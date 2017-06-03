#include <ace-c/Module.hpp>
#include <ace-c/Configuration.hpp>

#include <common/my_assert.hpp>

Module::Module(const std::string &name, const SourceLocation &location)
    : m_name(name),
      m_location(location),
      m_tree_link(nullptr)
{
}

bool Module::IsInFunction()
{
    TreeNode<Scope> *top = m_scopes.TopNode();
    
    while (top != nullptr) {
        if (top->m_value.GetScopeType() == SCOPE_TYPE_FUNCTION) {
            return true;
        }
        
        top = top->m_parent;
    }

    return false;
}

Module *Module::LookupNestedModule(const std::string &name)
{
    ASSERT(m_tree_link != nullptr);

    // search siblings of the current module,
    // rather than global lookup.
    for (auto *sibling : m_tree_link->m_siblings) {
        ASSERT(sibling != nullptr);
        ASSERT(sibling->m_value != nullptr);
        
        if (sibling->m_value->GetName() == name) {
            return sibling->m_value;
        }
    }

    return nullptr;
}

Identifier *Module::LookUpIdentifier(const std::string &name, bool this_scope_only)
{
    TreeNode<Scope> *top = m_scopes.TopNode();

    while (top != nullptr) {
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
    return PerformLookup(
        [&name](TreeNode<Scope> *top) {
            return top->m_value.GetIdentifierTable().LookupSymbolType(name);
        },
        [&name](Module *mod) {
            return mod->LookupSymbolType(name);
        }
    );
}

SymbolTypePtr_t Module::LookupGenericInstance(
    const SymbolTypePtr_t &base,
    const std::vector<GenericInstanceTypeInfo::Arg> &params)
{
    return PerformLookup(
        [&base, &params](TreeNode<Scope> *top) {
            return top->m_value.GetIdentifierTable().LookupGenericInstance(base, params);
        },
        [&base, &params](Module *mod) {
            return mod->LookupGenericInstance(base, params);
        }
    );
}

SymbolTypePtr_t Module::PerformLookup(
    std::function<SymbolTypePtr_t(TreeNode<Scope>*)> pred1,
    std::function<SymbolTypePtr_t(Module *mod)> pred2)
{
    TreeNode<Scope> *top = m_scopes.TopNode();

    while (top) {
        if (SymbolTypePtr_t result = pred1(top)) {
            // a result was found
            return result;
        }
        top = top->m_parent;
    }

    if (m_tree_link && m_tree_link->m_parent) {
        if (Module *other = m_tree_link->m_parent->m_value) {
            if (other->GetLocation().GetFileName() == m_location.GetFileName()) {
                return pred2(other);
            } else {
                // we are outside of file scope, so loop until root/global module found
                auto *link = m_tree_link->m_parent;

                while (link->m_parent) {
                    link = link->m_parent;
                }

                ASSERT(link->m_value != nullptr);
                ASSERT(link->m_value->GetName() == ace::compiler::Config::GLOBAL_MODULE_NAME);

                return pred2(link->m_value);
            }
        }
    }

    return nullptr;
}