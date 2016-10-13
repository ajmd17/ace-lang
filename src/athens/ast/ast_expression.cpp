#include <athens/ast/ast_expression.h>

AstExpression::AstExpression(const SourceLocation &location)
    : AstStatement(location),
      m_is_standalone(false)
{
}