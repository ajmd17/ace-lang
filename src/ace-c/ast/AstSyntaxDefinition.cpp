#include <ace-c/ast/AstSyntaxDefinition.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Lexer.hpp>
#include <ace-c/Parser.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/CompilationUnit.hpp>
#include <ace-c/Configuration.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

using namespace ace;

AstSyntaxDefinition::AstSyntaxDefinition(
    const std::shared_ptr<AstString> &syntax_string,
    const std::shared_ptr<AstString> &transform_string,
    const SourceLocation &location)
    : AstStatement(location),
      m_syntax_string(syntax_string),
      m_transform_string(transform_string)
{
}

void AstSyntaxDefinition::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_syntax_string != nullptr);
    ASSERT(m_transform_string != nullptr);

    std::cout << "syntax string: " << m_syntax_string->GetValue() << "\n"
      << "transform string: " << m_transform_string->GetValue() << "\n";
}

std::unique_ptr<Buildable> AstSyntaxDefinition::Build(AstVisitor *visitor, Module *mod)
{
    return nullptr;
}

void AstSyntaxDefinition::Optimize(AstVisitor *visitor, Module *mod)
{
}

Pointer<AstStatement> AstSyntaxDefinition::Clone() const
{
    return CloneImpl();
}
