#include <athens/ast_iterator.h>

AstIterator::AstIterator()
    : m_start(nullptr),
      m_current(nullptr),
      m_top(nullptr)
{
}

AstIterator::AstIterator(const AstIterator &other)
    : m_start(other.m_start),
      m_current(other.m_current),
      m_top(other.m_top)
{
}

void AstIterator::ResetPosition()
{
    m_current = m_start;
}

void AstIterator::PushBack(AstStatement *statement)
{
    if (m_start == nullptr) {
        m_start = statement;
    }

    if (m_current == nullptr) {
        m_current = statement;
    }

    if (m_top != nullptr) {
        m_top->m_next = statement;
    }

    m_top = statement;
}

AstStatement *AstIterator::Next()
{
    AstStatement *statement = m_current;
    m_current = statement->m_next;
    return statement;
}