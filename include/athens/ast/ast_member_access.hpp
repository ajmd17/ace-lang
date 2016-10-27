#ifndef AST_MEMBER_ACCESS_HPP
#define AST_MEMBER_ACCESS_HPP

#include <athens/ast/ast_identifier.hpp>

#include <string>
#include <vector>
#include <memory>

class AstMemberAccess : public AstExpression {
public:
    AstMemberAccess(std::shared_ptr<AstExpression> left,
        std::shared_ptr<AstExpression> right,
        const SourceLocation &location);
    virtual ~AstMemberAccess() = default;

    std::shared_ptr<AstExpression> &GetLeft() { return m_left; }
    const std::shared_ptr<AstExpression> &GetLeft() const { return m_left; }
    std::shared_ptr<AstExpression> &GetRight() { return m_right; }
    const std::shared_ptr<AstExpression> &GetRight() const { return m_right; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual ObjectType GetObjectType() const override;

private:
    std::shared_ptr<AstExpression> m_left;
    std::shared_ptr<AstExpression> m_right;

    // is m_left a module name
    Module *m_lhs_mod;
    // is it a free function call in object style
    bool m_is_free_call;
};

#endif
