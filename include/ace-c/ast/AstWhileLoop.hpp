#ifndef AST_WHILE_LOOP_HPP
#define AST_WHILE_LOOP_HPP

#include <ace-c/ast/AstStatement.hpp>
#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/ast/AstBlock.hpp>

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
    virtual void Recreate(std::ostringstream &ss) override;

private:
    std::shared_ptr<AstExpression> m_conditional;
    std::shared_ptr<AstBlock> m_block;
    int m_num_locals;
};

#endif
