#include <athens/ast/ast_true.h>

AstTrue::AstTrue(const SourceLocation &location)
    : AstConstant(location)
{
}

void AstTrue::Build(AstVisitor *visitor) const
{
}

int AstTrue::IsTrue() const
{
    return true;
}