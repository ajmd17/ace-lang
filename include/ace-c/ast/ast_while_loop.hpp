#ifndef AST_WHILE_LOOP_HPP
#define AST_WHILE_LOOP_HPP

#include <ace-c/ast/ast_statement.hpp>
#include <ace-c/ast/ast_expression.hpp>
#include <ace-c/ast/ast_block.hpp>

#include <memory>

class AstWhileLoop : public AstStatement {
public:
    AstWhileLoop(const std::shared_ptr<AstExpression> &conditional,
        const std::shared_ptr<AstBlock> &block,
        const SourceLocation &location);
    virtual ~AstWhileLoop() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;

private:
    std::shared_ptr<AstExpression> m_conditional;
    std::shared_ptr<AstBlock> m_block;
};

#endif
