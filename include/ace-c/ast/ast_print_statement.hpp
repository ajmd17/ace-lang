#ifndef AST_PRINT_STATEMENT_HPP
#define AST_PRINT_STATEMENT_HPP

#include <ace-c/ast/ast_statement.hpp>
#include <ace-c/ast/ast_expression.hpp>

#include <vector>
#include <memory>

class AstPrintStatement : public AstStatement {
public:
    AstPrintStatement(const std::vector<std::shared_ptr<AstExpression>> &arguments,
        const SourceLocation &location);
    virtual ~AstPrintStatement() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;

private:
    std::vector<std::shared_ptr<AstExpression>> m_arguments;
};

#endif
