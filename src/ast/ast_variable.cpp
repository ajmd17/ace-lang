#include <athens/ast/ast_variable.h>
#include <athens/ast_visitor.h>

AstVariable::AstVariable(const std::string &name, const SourceLocation &location)
    : AstStatement(location),
      m_name(name) 
{
}

void AstVariable::Visit(AstVisitor *visitor) 
{
    // make sure that the variable exists
    std::unique_ptr<Module> &mod = visitor->GetCompilationUnit()->CurrentModule();
    Scope &scope = mod->m_scopes.Top();

    // the variable must exist in the active scope or a parent scope
    Identifier *result = mod->LookUpIdentifier(m_name, false);
    if (result == nullptr) {
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_undeclared_identifier, m_location, m_name));
    } else {
        result->IncUseCount();
    }
}