#include <athens/optimizer.h>

Optimizer::Optimizer(const AstIterator &ast_iterator, CompilationUnit *compilation_unit)
    : AstVisitor(ast_iterator, compilation_unit)
{
}

Optimizer::Optimizer(const Optimizer &other)
    : AstVisitor(other.m_ast_iterator, other.m_compilation_unit)
{
}

void Optimizer::Optimize()
{
    while (m_ast_iterator.HasNext()) {
        m_ast_iterator.Next()->Optimize();
    }
}