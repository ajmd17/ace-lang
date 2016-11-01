#include <ace-c/ast_iterator.hpp>

AstIterator::AstIterator()
    : m_position(0)
{
}

AstIterator::AstIterator(const AstIterator &other)
    : m_position(other.m_position),
      m_list(other.m_list)
{
}
