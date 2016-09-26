#include "ast_declaration.h"

AstDeclaration::AstDeclaration(const std::string &name, const SourceLocation &location)
    : AstStatement(location), 
      m_name(name)
{
}