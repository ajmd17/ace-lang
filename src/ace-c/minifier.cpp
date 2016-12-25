#include <ace-c/minifier.hpp>

Minifier::Minifier(AstIterator *ast_iterator)
    : AstVisitor(ast_iterator, nullptr)
{
}

Minifier::Minifier(const Minifier &other)
    : AstVisitor(other.m_ast_iterator, nullptr)
{
}

void Minifier::Minify(std::ostringstream &ss)
{
    while (m_ast_iterator->HasNext()) {
        m_ast_iterator->Next()->Recreate(ss);
    }
}
