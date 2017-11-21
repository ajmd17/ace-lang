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

     m_string_expr.reset(new AstString(
         expr_type->GetName(),
         m_location
     ));
}

std::unique_ptr<Buildable> AstTypeOfExpression::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_string_expr != nullptr);
    return m_string_expr->Build(visitor, mod);
}

void AstTypeOfExpression::Optimize(AstVisitor *visitor, Module *mod)
{
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
    return false;
}

SymbolTypePtr_t AstTypeOfExpression::GetExprType() const
{
    ASSERT(m_expr != nullptr);
    return BuiltinTypes::STRING;
}