#include <ace-c/ast/AstArgument.hpp>
#include <ace-c/AstVisitor.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

AstArgument::AstArgument(
    const std::shared_ptr<AstExpression> &expr,
    bool is_named,
    const std::string &name,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_expr(expr),
      m_is_named(is_named),
      m_name(name)
{
}

void AstArgument::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);
    m_expr->Visit(visitor, mod);
}

std::unique_ptr<Buildable> AstArgument::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);
    return m_expr->Build(visitor, mod);
}

void AstArgument::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);
    m_expr->Optimize(visitor, mod);
}

Pointer<AstStatement> AstArgument::Clone() const
{
    return CloneImpl();
}

int AstArgument::IsTrue() const
{
    ASSERT(m_expr != nullptr);
    return m_expr->IsTrue();
}

bool AstArgument::MayHaveSideEffects() const
{
    ASSERT(m_expr != nullptr);
    return m_expr->MayHaveSideEffects();
}

SymbolTypePtr_t AstArgument::GetSymbolType() const
{
    ASSERT(m_expr != nullptr);
    return m_expr->GetSymbolType();
}