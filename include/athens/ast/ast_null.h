#ifndef AST_NULL_H
#define AST_NULL_H

#include <athens/ast/ast_expression.h>

class AstNull : public AstExpression {
public:
    AstNull(const SourceLocation &location);

    virtual void Visit(AstVisitor *visitor);
    virtual void Build(AstVisitor *visitor) const;
    virtual void Optimize();
    virtual int IsTrue() const;
};

#endif