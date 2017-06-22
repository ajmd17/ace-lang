#ifndef AST_YIELD_STATEMENT_HPP
#define AST_YIELD_STATEMENT_HPP

#include <ace-c/ast/AstStatement.hpp>
#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/ast/AstCallExpression.hpp>

#include <memory>

class AstYieldStatement : public AstStatement {
public:
    AstYieldStatement(const std::shared_ptr<AstExpression> &expr,
        const SourceLocation &location);
    virtual ~AstYieldStatement() = default;

    inline const std::shared_ptr<AstExpression> &GetExpression() const
        { return m_expr; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;
    virtual Pointer<AstStatement> Clone() const override;

private:
    std::shared_ptr<AstExpression> m_expr;
    int m_num_pops;

    std::shared_ptr<AstCallExpression> m_yield_callback_call;

    inline Pointer<AstYieldStatement> CloneImpl() const
    {
        return Pointer<AstYieldStatement>(new AstYieldStatement(
            CloneAstNode(m_expr),
            m_location
        ));
    }
};

#endif
