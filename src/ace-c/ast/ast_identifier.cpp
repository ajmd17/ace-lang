#include <ace-c/ast/ast_identifier.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/module.hpp>

#include <iostream>

#include <cassert>

AstIdentifier::AstIdentifier(const std::string &name, const SourceLocation &location)
    : AstExpression(location),
      m_name(name),
      m_identifier(nullptr),
      m_access_mode(ACCESS_MODE_LOAD),
      m_in_function(false)
{
}

void AstIdentifier::Visit(AstVisitor *visitor, Module *mod)
{
    // make sure that the variable exists
    Scope &scope = mod->m_scopes.Top();

    // the variable must exist in the active scope or a parent scope
    m_identifier = mod->LookUpIdentifier(m_name, false);
    if (m_identifier == nullptr) {
        bool found_module = false;
        // check all modules for one with the same name
        for (const auto &it : visitor->GetCompilationUnit()->m_modules) {
            if (it != nullptr && it->GetName() == m_name) {
                // module with name found
                found_module = true;
                break;
            }
        }

        if (found_module) {
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_identifier_is_module, m_location, m_name));
        } else {
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_undeclared_identifier, m_location, m_name));
        }
    } else {
        m_identifier->IncUseCount();
    }

    TreeNode<Scope> *top = mod->m_scopes.TopNode();
    while (top != nullptr) {
        if (top->m_value.GetScopeType() == SCOPE_TYPE_FUNCTION) {
            m_in_function = true;
            break;
        }
        top = top->m_parent;
    }
}

ObjectType AstIdentifier::GetObjectType() const
{
    if (m_identifier != nullptr) {
        return m_identifier->GetObjectType();
    }
    return ObjectType::type_builtin_undefined;
}

int AstIdentifier::GetStackOffset(int stack_size) const
{
    assert(m_identifier != nullptr);
    return stack_size - m_identifier->GetStackLocation();
}
