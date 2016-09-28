#ifndef AST_CONSTANT_H
#define AST_CONSTANT_H

#include <athens/ast/ast_expression.h>

class AstConstant : public AstExpression {
public:
    AstConstant(const SourceLocation &location);

    virtual void Visit(AstVisitor *visitor);
    virtual void Build(AstVisitor *visitor) const = 0;
    virtual void Optimize();
    virtual int IsTrue() const = 0;
};

#endif