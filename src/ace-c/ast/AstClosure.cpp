#include <ace-c/ast/AstClosure.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/Module.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

#include <iostream>

AstClosure::AstClosure(const std::shared_ptr<AstFunctionExpression> &expr,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_expr(expr)
{
}

void AstClosure::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_expr != nullptr);
    m_expr->Visit(visitor, mod);
}

void AstClosure::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_expr != nullptr);
    m_expr->Build(visitor, mod);
}

void AstClosure::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_expr != nullptr);
    m_expr->Optimize(visitor, mod);
}

void AstClosure::Recreate(std::ostringstream &ss)
{
}

Pointer<AstStatement> AstClosure::Clone() const
{
    return CloneImpl();
}

int AstClosure::IsTrue() const
{
    return 1;
}

bool AstClosure::MayHaveSideEffects() const
{
    return true;
}

SymbolTypePtr_t AstClosure::GetSymbolType() const
{
    ASSERT(m_expr != nullptr);
    return m_expr->GetSymbolType();
}