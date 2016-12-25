#include <ace-c/ast/ast_identifier.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/module.hpp>

#include <iostream>

#include <common/my_assert.hpp>

AstIdentifier::AstIdentifier(const std::string &name, const SourceLocation &location)
    : AstExpression(location),
      m_name(name)
{
}

void AstIdentifier::PerformLookup(AstVisitor *visitor, Module *mod)
{
    ObjectType tmp;

    // the variable must exist in the active scope or a parent scope
    if (m_properties.identifier = mod->LookUpIdentifier(m_name, false)) {
        m_properties.identifier_type = IDENTIFIER_TYPE_VARIABLE;
    } else if (m_properties.identifier = visitor->GetCompilationUnit()->GetGlobalModule()->LookUpIdentifier(m_name, false)) {
        // if the identifier was not found,
        // look in the global module to see if it is a global function.
        m_properties.identifier_type = IDENTIFIER_TYPE_VARIABLE;
    } else if (visitor->GetCompilationUnit()->LookupModule(m_name)) {
        m_properties.identifier_type = IDENTIFIER_TYPE_MODULE;
    } else if (ObjectType::GetBuiltinType(m_name)) {
        m_properties.identifier_type = IDENTIFIER_TYPE_TYPE;
    } else if (mod->LookUpUserType(m_name, tmp)) {
        m_properties.identifier_type = IDENTIFIER_TYPE_TYPE;
    } else if (visitor->GetCompilationUnit()->GetGlobalModule()->LookUpUserType(m_name, tmp)) {
        m_properties.identifier_type = IDENTIFIER_TYPE_TYPE;
    } else {
        // nothing was found
        m_properties.identifier_type = IDENTIFIER_TYPE_NOT_FOUND;
    }
}

void AstIdentifier::CheckInFunction(AstVisitor *visitor, Module *mod)
{
    m_properties.depth = 0;
    TreeNode<Scope> *top = mod->m_scopes.TopNode();
    while (top) {
        m_properties.depth++;
        if (top->m_value.GetScopeType() == SCOPE_TYPE_FUNCTION) {
            m_properties.is_in_function = true;
            break;
        }
        top = top->m_parent;
    }
}

void AstIdentifier::Visit(AstVisitor *visitor, Module *mod)
{
    if (m_properties.identifier_type == IDENTIFIER_TYPE_UNKNOWN) {
        PerformLookup(visitor, mod);
    }

    CheckInFunction(visitor, mod);
}

ObjectType AstIdentifier::GetObjectType() const
{
    if (m_properties.identifier) {
        return m_properties.identifier->GetObjectType();
    }
    return ObjectType::type_builtin_undefined;
}

int AstIdentifier::GetStackOffset(int stack_size) const
{
    ASSERT(m_properties.identifier != nullptr);
    return stack_size - m_properties.identifier->GetStackLocation();
}
