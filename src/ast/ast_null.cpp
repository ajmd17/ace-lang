#include <athens/ast/ast_null.h>

AstNull::AstNull(const SourceLocation &location)
    : AstExpression(location)
{
}

void AstNull::Visit(AstVisitor *visitor)
{
    // do nothing
}

void AstNull::Build(AstVisitor *visitor) const
{
}

void AstNull::Optimize()
{
    // do nothing
}

int AstNull::IsTrue() const
{
    return false;
}
