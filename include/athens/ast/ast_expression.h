#ifndef AST_EXPRESSION_H
#define AST_EXPRESSION_H

#include <athens/ast/ast_statement.h>

class Expression : public AstStatement {
public:
    Expression(const SourceLocation &location);
    virtual ~Expression() = default;
};

#endif