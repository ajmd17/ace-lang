#include <ace-c/ast/AstTypeOfExpression.hpp>
#include <ace-c/ast/AstString.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Configuration.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/my_assert.hpp>

AstTypeOfExpression::AstTypeOfExpression(
    const std::shared_ptr<AstExpression> &expr,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_expr(expr),
      m_static_id(0)
{
}

void AstTypeOfExpression::Visit(AstVisitor *visitor, Module *mod)
{
     ASSERT(m_expr != nullptr);
     m_expr->Visit(visitor, mod);
    
     SymbolTypePtr_t expr_type = m_expr->GetExprType();
     ASSERT(expr_type != nullptr);

#if 0
     if (expr_type == BuiltinTypes::ANY) {
        // add runtime::typeof call
        m_runtime_typeof_call = visitor->GetCompilationUnit()->GetAstNodeBuilder()
            .Module("runtime")
            .Function("typeof")
            .Call({
                std::shared_ptr<AstArgument>((new AstArgument(
                    m_expr,
                    false,
                    "",
                    SourceLocation::eof
                )))
            });

        ASSERT(m_runtime_typeof_call != nullptr);
        m_runtime_typeof_call->Visit(visitor, mod);
    }
#endif
}

std::unique_ptr<Buildable> AstTypeOfExpression::Build(AstVisitor *visitor, Module *mod)
{
    //ASSERT(m_runtime_typeof_call != nullptr);
    /*if (m_runtime_typeof_call != nullptr) {
        return m_runtime_typeof_call->Build(visitor, mod);
    } else {*/
        ASSERT(m_expr != nullptr);

        SymbolTypePtr_t expr_type = m_expr->GetExprType();
        ASSERT(expr_type != nullptr);

        // simply add a string representing the type

        // get active register
        uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        auto instr_string = BytecodeUtil::Make<BuildableString>();
        instr_string->reg = rp;
        instr_string->value = expr_type->GetName();
        return std::move(instr_string);
    //}
}

void AstTypeOfExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    if (m_runtime_typeof_call != nullptr) {
        m_runtime_typeof_call->Optimize(visitor, mod);
    } else {
        ASSERT(m_expr != nullptr);
        m_expr->Optimize(visitor, mod);
    }
}

Pointer<AstStatement> AstTypeOfExpression::Clone() const
{
    return CloneImpl();
}

Tribool AstTypeOfExpression::IsTrue() const
{
    return Tribool::True();
}

bool AstTypeOfExpression::MayHaveSideEffects() const
{
    ASSERT(m_expr != nullptr);

    return m_expr->MayHaveSideEffects();
}

SymbolTypePtr_t AstTypeOfExpression::GetExprType() const
{
    ASSERT(m_expr != nullptr);
    
    return BuiltinTypes::STRING;
}