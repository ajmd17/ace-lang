#include <athens/ast/ast_declaration.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/module.hpp>

AstDeclaration::AstDeclaration(const std::string &name, const SourceLocation &location)
    : AstStatement(location),
      m_name(name),
      m_identifier(nullptr)
{
}

void AstDeclaration::Visit(AstVisitor *visitor, Module *mod)
{
    CompilationUnit *compilation_unit = visitor->GetCompilationUnit();
    Scope &scope = mod->m_scopes.Top();

    // look up variable to make sure it doesn't already exist
    // only this scope matters, variables with the same name outside
    // of this scope are fine
    m_identifier = mod->LookUpIdentifier(m_name, true);
    if (m_identifier != nullptr) {
        // a collision was found, add an error
        compilation_unit->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_redeclared_identifier, m_location, m_name));
    } else {
        // add identifier
        m_identifier = scope.GetIdentifierTable().AddIdentifier(m_name);
    }
}

ObjectType AstDeclaration::GetObjectType() const
{
    if (m_identifier != nullptr) {
        return m_identifier->GetObjectType();
    }
    return ObjectType::type_builtin_undefined;
}
