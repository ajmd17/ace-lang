#include <athens/ast_iterator.hpp>

AstIterator::AstIterator()
    : m_position(0)
{
}

AstIterator::AstIterator(const AstIterator &other)
    : m_position(other.m_position),
      m_list(other.m_list)
{
}

void AstIterator::ResetPosition()
{
    m_position = 0;
}

void AstIterator::Push(const std::shared_ptr<AstStatement> &statement)
{
    m_list.push_back(statement);
}

std::shared_ptr<AstStatement> AstIterator::Next()
{
    return m_list[m_position++];
}
