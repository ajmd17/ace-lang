#ifndef AST_BINARY_EXPRESSION_HPP
#define AST_BINARY_EXPRESSION_HPP

#include <ace-c/ast/ast_expression.hpp>
#include <ace-c/ast/ast_member_access.hpp>
#include <ace-c/ast/ast_variable_declaration.hpp>
#include <ace-c/operator.hpp>

class AstBinaryExpression : public AstExpression {
public:
    AstBinaryExpression(const std::shared_ptr<AstExpression> &left,
        const std::shared_ptr<AstExpression> &right,
        const Operator *op,
        const SourceLocation &location);

    inline const std::shared_ptr<AstExpression> &GetLeft() const { return m_left; }
    inline const std::shared_ptr<AstExpression> &GetRight() const { return m_right; }

    bool VisitOperatorOverload(const ObjectType &left_type, const ObjectType &right_type);

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual ObjectType GetObjectType() const override;

private:
    std::shared_ptr<AstExpression> m_left;
    std::shared_ptr<AstExpression> m_right;
    const Operator *m_op;

    // if the operator is overloaded and it is actually a function call
    std::shared_ptr<AstMemberAccess> m_member_access;

    // if the expression is lazy declaration
    std::shared_ptr<AstVariableDeclaration> m_variable_declaration;

    std::shared_ptr<AstVariableDeclaration> CheckLazyDeclaration(AstVisitor *visitor, Module *mod);
};

#endif
