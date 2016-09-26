#include "ast_braces.h"
#include "../ast_visitor.h"

AstOpenBrace::AstOpenBrace(const SourceLocation &location)
    : AstStatement(location)
{
}

void AstOpenBrace::Visit(AstVisitor *visitor)
{
}

AstCloseBrace::AstCloseBrace(const SourceLocation &location)
    : AstStatement(location)
{
}

void AstCloseBrace::Visit(AstVisitor *visitor)
{
    //visitor->GetCompilationUnit()->CurrentModule()->
}