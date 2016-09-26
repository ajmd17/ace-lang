#ifndef AST_BRACES_H
#define AST_BRACES_H

#include <athens/ast/ast_statement.h>

class AstOpenBrace : public AstStatement {
public:
    AstOpenBrace(const SourceLocation &location);

    virtual void Visit(AstVisitor *visitor);
};

class AstCloseBrace : public AstStatement {
public:
    AstCloseBrace(const SourceLocation &location);

    virtual void Visit(AstVisitor *visitor);
};

#endif