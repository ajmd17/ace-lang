#include <athens/ast/ast_import.h>
#include <athens/ast_visitor.h>
#include <athens/optimizer.h>
#include <athens/compiler.h>

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