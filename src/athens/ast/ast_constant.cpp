#include <athens/ast/ast_constant.hpp>

AstConstant::AstConstant(const SourceLocation &location)
    : AstExpression(location)
{
}

void AstConstant::Visit(AstVisitor *visitor, Module *mod)
{
    // do nothing
}

void AstConstant::Optimize(AstVisitor *visitor, Module *mod)
{
    // do nothing
}

bool AstConstant::MayHaveSideEffects() const
{
    // constants do not have side effects
    return false;
}
