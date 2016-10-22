#include <athens/ast/ast_parameter.hpp>
#include <athens/ast_visitor.hpp>

AstParameter::AstParameter(const std::string &name, bool is_variadic,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_is_variadic(is_variadic)
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
