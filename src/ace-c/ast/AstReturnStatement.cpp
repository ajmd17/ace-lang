#include <ace-c/ast/AstReturnStatement.hpp>
#include <ace-c/Optimizer.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Keywords.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

AstReturnStatement::AstReturnStatement(const std::shared_ptr<AstExpression> &expr,
    const SourceLocation &location)
    : AstStatement(location),
      m_expr(expr),
      m_num_pops(0)
{
}

void AstReturnStatement::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);
    m_expr->Visit(visitor, mod);

    // transverse the scope tree to make sure we are in a function
    bool in_function = false;

    TreeNode<Scope> *top = mod->m_scopes.TopNode();
    while (top != nullptr) {
        if (top->m_value.GetScopeType() == SCOPE_TYPE_FUNCTION) {
            in_function = true;
            break;
        }
        m_num_pops += top->m_value.GetIdentifierTable().CountUsedVariables();
        top = top->m_parent;
    }

    if (in_function) {
        ASSERT(top != nullptr);
        // add return type
        top->m_value.AddReturnType(m_expr->GetSymbolType(), m_location);
    } else {
        // error; 'return' not allowed outside of a function
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_return_outside_function,
                m_location));
    }
}

void AstReturnStatement::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);
    m_expr->Build(visitor, mod);

    // pop all variables in the way off of the stack
    for (int i = 0; i < m_num_pops; i++) {
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t>(POP);
    }

    // add RET instruction
    visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t>(RET);
}

void AstReturnStatement::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);
    m_expr->Optimize(visitor, mod);
}

void AstReturnStatement::Recreate(std::ostringstream &ss)
{
    ASSERT(m_expr != nullptr);
    ss << Keyword::ToString(Keyword_return) << " ";
    m_expr->Recreate(ss);
}

Pointer<AstStatement> AstReturnStatement::Clone() const
{
    return CloneImpl();
}
