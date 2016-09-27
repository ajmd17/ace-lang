#ifndef AST_IF_STATEMENT_H
#define AST_IF_STATEMENT_H

#include <athens/ast/ast_statement.h>
#include <athens/ast/ast_expression.h>
#include <athens/ast/ast_block.h>

#include <memory>

class AstIfStatement : public AstStatement {
public:
    AstIfStatement(std::unique_ptr<AstExpression> &&conditional, 
        std::unique_ptr<AstBlock> &&block,
        const SourceLocation &location);
    virtual ~AstIfStatement() = default;

    virtual void Visit(AstVisitor *visitor);

private:
    std::unique_ptr<AstExpression> m_conditional;
    std::unique_ptr<AstBlock> m_block;
};

#endif