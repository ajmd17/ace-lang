#include <athens/ast/ast_parameter.h>
#include <athens/ast_visitor.h>

AstParameter::AstParameter(const std::string &name, const SourceLocation &location)
    : AstDeclaration(name, location)
{
}

void AstParameter::Visit(AstVisitor *visitor)
{
    AstDeclaration::Visit(visitor);
}

void AstParameter::Build(AstVisitor *visitor) const
{
}

void AstParameter::Optimize(AstVisitor *visitor)
{
}
