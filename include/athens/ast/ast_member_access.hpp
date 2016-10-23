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

    virtual void Visit(AstVisitor *visitor) override;
    virtual void Build(AstVisitor *visitor) override;
    virtual void Optimize(AstVisitor *visitor) override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;

private:
    std::shared_ptr<AstExpression> m_left;
    std::shared_ptr<AstExpression> m_right;

    // is m_left a module name
    bool m_lhs_mod;
    // module index of left-hand side
    int m_mod_index;
    // original module index
    int m_mod_index_before;
};

#endif
