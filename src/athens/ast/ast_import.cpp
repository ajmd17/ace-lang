#include <athens/ast/ast_import.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/optimizer.hpp>
#include <athens/compiler.hpp>

AstImport::AstImport(const SourceLocation &location)
    : AstStatement(location)
{
}

void AstImport::Build(AstVisitor *visitor)
{
    m_ast_iterator.ResetPosition();

    // compile the imported module
    Compiler compiler(&m_ast_iterator, visitor->GetCompilationUnit());
    compiler.Compile();
}

void AstImport::Optimize(AstVisitor *visitor)
{
    m_ast_iterator.ResetPosition();

    // optimize the imported module
    Optimizer optimizer(&m_ast_iterator, visitor->GetCompilationUnit());
    optimizer.Optimize();
}
