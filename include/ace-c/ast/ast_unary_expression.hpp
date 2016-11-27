#ifndef AST_UNARY_EXPRESSION_HPP
#define AST_UNARY_EXPRESSION_HPP

#include <ace-c/ast/ast_expression.hpp>
#include <ace-c/ast/ast_member_access.hpp>
#include <ace-c/operator.hpp>

class AstUnaryExpression : public AstExpression {
public:
    AstUnaryExpression(const std::shared_ptr<AstExpression> &target,
        const Operator *op,
        const SourceLocation &location);

    inline const std::shared_ptr<AstExpression> &GetTarget() const { return m_target; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual ObjectType GetObjectType() const override;

private:
    std::shared_ptr<AstExpression> m_target;
    const Operator *m_op;
    bool m_folded;
};

#endif