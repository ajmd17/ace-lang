#include <ace-c/module.hpp>

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
        Identifier *result =
            top->m_value.GetIdentifierTable().LookUpIdentifier(name);

        if (result) {
            // a result was found
            return result;
        }

        if (this_scope_only) {
            break;
        }

        top = top->m_parent;
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

bool Module::LookUpUserType(const std::string &type, ObjectType &out)
{
    for (ObjectType &it : m_user_types) {
        if (it.ToString() == type) {
            out = it;
            return true;
        }
    }

    return false;
}

void Module::AddUserType(const ObjectType &type)
{
    m_user_types.push_back(type);
}