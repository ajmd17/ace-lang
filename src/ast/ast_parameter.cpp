#include <athens/ast/ast_parameter.h>
#include <athens/ast_visitor.h>

AstParameter::AstParameter(const std::string &name, const SourceLocation &location)
    : AstDeclaration(name, location)
{
}

void AstParameter::Visit(AstVisitor *visitor)
{
    std::unique_ptr<Module> &mod = visitor->GetCompilationUnit()->CurrentModule();
    Scope &scope = mod->m_scopes.Top();

    // look up variable to make sure it doesn't already exist
    // only this scope matters, variables with the same name outside
    // of this scope are fine
    Identifier *result = mod->LookUpIdentifier(m_name, true);
    if (result != nullptr) {
        // a collision was found, add an error
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_redeclared_identifier, m_location, m_name));
    } else {
        // add identifier
        scope.GetIdentifierTable().AddIdentifier(m_name);
    }
}