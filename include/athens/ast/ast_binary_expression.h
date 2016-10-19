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

    inline const std::shared_ptr<AstExpression> &GetLeft() const { return m_left; }
    inline const std::shared_ptr<AstExpression> &GetRight() const { return m_right; }

    virtual void Visit(AstVisitor *visitor) override;
    virtual void Build(AstVisitor *visitor) override;
    virtual void Optimize(AstVisitor *visitor) override;
    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;

private:
    std::shared_ptr<AstExpression> m_left;
    std::shared_ptr<AstExpression> m_right;
    const Operator *m_op;
};

#endif
