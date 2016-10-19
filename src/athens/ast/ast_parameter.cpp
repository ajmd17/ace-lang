#include <athens/ast/ast_parameter.hpp>
#include <athens/ast_visitor.hpp>

AstParameter::AstParameter(const std::string &name, const SourceLocation &location)
    : AstDeclaration(name, location)
{
}

void AstParameter::Visit(AstVisitor *visitor)
{
    AstDeclaration::Visit(visitor);
}

void AstParameter::Build(AstVisitor *visitor)
{
}

void AstParameter::Optimize(AstVisitor *visitor)
{
}
