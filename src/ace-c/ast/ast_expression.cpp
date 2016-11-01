#include <ace-c/ast/ast_expression.hpp>

AstExpression::AstExpression(const SourceLocation &location)
    : AstStatement(location),
      m_is_standalone(false)
{
}
