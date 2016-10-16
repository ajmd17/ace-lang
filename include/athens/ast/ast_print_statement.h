#ifndef AST_PRINT_STATEMENT_H
#define AST_PRINT_STATEMENT_H

#include <athens/ast/ast_statement.h>
#include <athens/ast/ast_expression.h>

#include <vector>
#include <memory>

class AstPrintStatement : public AstStatement {
public:
    AstPrintStatement(const std::vector<std::shared_ptr<AstExpression>> &arguments,
        const SourceLocation &location);
    virtual ~AstPrintStatement() = default;

    virtual void Visit(AstVisitor *visitor) override;
    virtual void Build(AstVisitor *visitor) override;
    virtual void Optimize(AstVisitor *visitor) override;

private:
    std::vector<std::shared_ptr<AstExpression>> m_arguments;
};

#endif