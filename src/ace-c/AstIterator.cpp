#include <ace-c/AstIterator.hpp>
#include <ace-c/ast/AstVariableDeclaration.hpp>
#include <ace-c/ast/AstTypeObject.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

AstIterator::AstIterator()
    : m_position(0)
{
}

AstIterator::AstIterator(const AstIterator &other)
    : m_position(other.m_position),
      m_list(other.m_list)
{
}
