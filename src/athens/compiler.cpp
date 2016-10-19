#include <athens/compiler.hpp>

Compiler::Compiler(AstIterator *ast_iterator, CompilationUnit *compilation_unit)
    : AstVisitor(ast_iterator, compilation_unit)
{
}

Compiler::Compiler(const Compiler &other)
    : AstVisitor(other.m_ast_iterator, other.m_compilation_unit)
{
}

void Compiler::Compile()
{
    while (m_ast_iterator->HasNext()) {
        m_ast_iterator->Next()->Build(this);
    }
}
