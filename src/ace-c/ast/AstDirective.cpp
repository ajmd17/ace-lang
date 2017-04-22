#include <ace-c/ast/AstDirective.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/CompilerError.hpp>

#include <common/my_assert.hpp>

AstDirective::AstDirective(const std::string &key,
    const std::string &value,
    const SourceLocation &location)
    : AstStatement(location),
      m_key(key),
      m_value(value)
{
}

void AstDirective::Visit(AstVisitor *visitor, Module *mod)
{
    
}

void AstDirective::Build(AstVisitor *visitor, Module *mod)
{

}

void AstDirective::Optimize(AstVisitor *visitor, Module *mod)
{

}

void AstDirective::Recreate(std::ostringstream &ss)
{

}

Pointer<AstStatement> AstDirective::Clone() const
{
    return CloneImpl();
}
