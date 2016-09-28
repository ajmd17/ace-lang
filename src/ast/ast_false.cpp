#include <athens/ast/ast_false.h>

AstFalse::AstFalse(const SourceLocation &location)
    : AstConstant(location)
{
}

void AstFalse::Build(AstVisitor *visitor) const
{
}

int AstFalse::IsTrue() const
{
    return false;
}