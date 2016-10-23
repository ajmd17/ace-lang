#include <athens/ast/ast_identifier.hpp>
#include <athens/ast_visitor.hpp>

AstIdentifier::AstIdentifier(const std::string &name, const SourceLocation &location)
    : AstExpression(location),
      m_name(name),
      m_identifier(nullptr)
{
}

void AstIdentifier::Visit(AstVisitor *visitor)
{
    // make sure that the variable exists
    std::unique_ptr<Module> &mod = visitor->GetCompilationUnit()->CurrentModule();
    Scope &scope = mod->m_scopes.Top();

    // the variable must exist in the active scope or a parent scope
    m_identifier = mod->LookUpIdentifier(m_name, false);
    if (m_identifier == nullptr) {
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_undeclared_identifier, m_location, m_name));
    } else {
        m_identifier->IncUseCount();
    }
}
