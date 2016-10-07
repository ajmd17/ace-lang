#include <athens/ast/ast_function_call.h>
#include <athens/ast_visitor.h>
#include <athens/ast/ast_constant.h>

AstFunctionCall::AstFunctionCall(const std::string &name, 
        const std::vector<std::shared_ptr<AstExpression>> &args, const SourceLocation &location)
    : AstExpression(location),
      m_name(name),
      m_args(args),
      m_identifier(nullptr)
{
}

void AstFunctionCall::Visit(AstVisitor *visitor) 
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

void AstFunctionCall::Build(AstVisitor *visitor) const
{
}

void AstFunctionCall::Optimize(AstVisitor *visitor)
{
}

int AstFunctionCall::IsTrue() const
{
    // cannot deduce if return value is true
    return -1;
}