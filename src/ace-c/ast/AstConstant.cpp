#include <ace-c/ast/AstConstant.hpp>

AstConstant::AstConstant(const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD)
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
