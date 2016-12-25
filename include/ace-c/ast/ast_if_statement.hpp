#ifndef AST_IF_STATEMENT_HPP
#define AST_IF_STATEMENT_HPP

#include <ace-c/ast/ast_statement.hpp>
#include <ace-c/ast/ast_expression.hpp>
#include <ace-c/ast/ast_block.hpp>

#include <memory>

class AstIfStatement : public AstStatement {
public:
    AstIfStatement(const std::shared_ptr<AstExpression> &conditional,
        const std::shared_ptr<AstBlock> &block,
        const std::shared_ptr<AstBlock> &else_block,
        const SourceLocation &location);
    virtual ~AstIfStatement() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;

private:
    std::shared_ptr<AstExpression> m_conditional;
    std::shared_ptr<AstBlock> m_block;
    std::shared_ptr<AstBlock> m_else_block;
};

#endif
