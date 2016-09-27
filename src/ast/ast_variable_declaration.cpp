#include <athens/ast/ast_variable_declaration.h>
#include <athens/ast_visitor.h>

AstVariableDeclaration::AstVariableDeclaration(const std::string &name, std::unique_ptr<AstExpression> &assignment,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_assignment(std::move(assignment))
{
}

void AstVariableDeclaration::Visit(AstVisitor *visitor)
{
    std::unique_ptr<Module> &mod = visitor->GetCompilationUnit()->CurrentModule();
    Scope &scope = mod->m_scopes.Top();

    // look up variable to make sure it doesn't already exist
    // only this scope matters, variables with the same name outside
    // of this scope are fine
    const Identifier *result = mod->LookUpIdentifier(m_name, true);
    if (result != nullptr) {
        // a collision was found, add an error
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_redeclared_identifier, m_location, m_name));
    } else {
        // add identifier
        scope.GetIdentifierTable().AddIdentifier(m_name);

        // if there was an assignment, visit it
        if (m_assignment != nullptr) {
            m_assignment->Visit(visitor);
        }
    }
}