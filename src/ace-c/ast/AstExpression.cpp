#include <ace-c/ast/AstExpression.hpp>

AstExpression::AstExpression(const SourceLocation &location)
    : AstStatement(location),
      m_is_standalone(false)
{
}
