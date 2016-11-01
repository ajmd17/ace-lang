#include <ace-c/ast/ast_import.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/optimizer.hpp>
#include <ace-c/compiler.hpp>

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
