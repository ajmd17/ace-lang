#ifndef AST_RETURN_STATEMENT_HPP
#define AST_RETURN_STATEMENT_HPP

#include <ace-c/ast/ast_statement.hpp>
#include <ace-c/ast/ast_expression.hpp>

#include <memory>

class AstReturnStatement : public AstStatement {
public:
    AstReturnStatement(const std::shared_ptr<AstExpression> &expr,
        const SourceLocation &location);
    virtual ~AstReturnStatement() = default;

    inline const std::shared_ptr<AstExpression> &GetExpression() const
        { return m_expr; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;

private:
    std::shared_ptr<AstExpression> m_expr;
    int m_num_pops;
};



#endif
