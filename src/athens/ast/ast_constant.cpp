#include <athens/ast/ast_constant.h>

AstConstant::AstConstant(const SourceLocation &location)
    : AstExpression(location)
{
}

void AstConstant::Visit(AstVisitor *visitor)
{
    // do nothing
}

void AstConstant::Optimize(AstVisitor *visitor)
{
    // do nothing
}

bool AstConstant::MayHaveSideEffects() const
{
    // constants do not have side effects
    return false;
}
