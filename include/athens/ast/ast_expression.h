#ifndef AST_EXPRESSION_H
#define AST_EXPRESSION_H

#include <athens/ast/ast_statement.h>

class AstExpression : public AstStatement {
public:
    AstExpression(const SourceLocation &location);
    virtual ~AstExpression() = default;

    virtual void Visit(AstVisitor *visitor) = 0;
};

#endif