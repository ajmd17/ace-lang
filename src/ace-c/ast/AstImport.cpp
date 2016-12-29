#include <ace-c/ast/AstImport.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Optimizer.hpp>
#include <ace-c/Compiler.hpp>

AstImport::AstImport(const SourceLocation &location)
    : AstStatement(location)
{
}

void AstImport::Build(AstVisitor *visitor, Module *mod)
{
    m_ast_iterator.ResetPosition();

    // compile the imported module
    Compiler compiler(&m_ast_iterator, visitor->GetCompilationUnit());
    compiler.Compile();
}

void AstImport::Optimize(AstVisitor *visitor, Module *mod)
{
    m_ast_iterator.ResetPosition();

    // optimize the imported module
    Optimizer optimizer(&m_ast_iterator, visitor->GetCompilationUnit());
    optimizer.Optimize();
}
