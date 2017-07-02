#include <ace-c/ast/AstYieldStatement.hpp>
#include <ace-c/ast/AstVariable.hpp>
#include <ace-c/ast/AstArgument.hpp>
#include <ace-c/Optimizer.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Keywords.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

AstYieldStatement::AstYieldStatement(const std::shared_ptr<AstExpression> &expr,
    const SourceLocation &location)
    : AstStatement(location),
      m_expr(expr),
      m_num_pops(0)
{
}

void AstYieldStatement::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_expr != nullptr);
    
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

    if (top != nullptr && in_function) {
        m_yield_callback_call.reset(new AstCallExpression(
            std::shared_ptr<AstVariable>(new AstVariable(
                "__generator_callback",
                m_location
            )),
            { std::shared_ptr<AstArgument>(new AstArgument(
                m_expr,
                false,
                "",
                m_location
             )) },
            false,
            m_location
        ));

        m_yield_callback_call->Visit(visitor, mod);
    } else {
        // error; 'yield' not allowed outside of a function
        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
            LEVEL_ERROR,
            Msg_yield_outside_function,
            m_location
        ));
    }
}

std::unique_ptr<Buildable> AstYieldStatement::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_yield_callback_call != nullptr);
    return m_yield_callback_call->Build(visitor, mod);
}

void AstYieldStatement::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_yield_callback_call != nullptr);

    m_yield_callback_call->Optimize(visitor, mod);
}

void AstYieldStatement::Recreate(std::ostringstream &ss)
{
    ASSERT(m_expr != nullptr);

    ss << Keyword::ToString(Keyword_yield) << " ";
    m_expr->Recreate(ss);
}

Pointer<AstStatement> AstYieldStatement::Clone() const
{
    return CloneImpl();
}
