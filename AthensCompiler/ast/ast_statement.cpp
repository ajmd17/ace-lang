#include "ast_statement.h"

AstStatement::AstStatement(const SourceLocation &location)
    : m_location(location), 
      m_next(nullptr)
{
}