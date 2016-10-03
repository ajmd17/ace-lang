#ifndef AST_BINARY_EXPRESSION_H
#define AST_BINARY_EXPRESSION_H

#include <athens/ast/ast_expression.h>
#include <athens/operator.h>

class AstBinaryExpression : public AstExpression {
public:
    AstBinaryExpression(const std::shared_ptr<AstExpression> &left,
        const std::shared_ptr<AstExpression> &right,
        const Operator *op,
        const SourceLocation &location);

    virtual void Visit(AstVisitor *visitor);
    virtual void Build(AstVisitor *visitor) const;
    virtual void Optimize(AstVisitor *visitor);
    virtual int IsTrue() const;

private:
    std::shared_ptr<AstExpression> m_left;
    std::shared_ptr<AstExpression> m_right;
    const Operator *m_op;
};

#endif