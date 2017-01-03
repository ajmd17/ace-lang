#ifndef AST_PRINT_STATEMENT_HPP
#define AST_PRINT_STATEMENT_HPP

#include <ace-c/ast/AstStatement.hpp>
#include <ace-c/ast/AstExpression.hpp>

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
    virtual Pointer<AstStatement> Clone() const override;

private:
    std::vector<std::shared_ptr<AstExpression>> m_arguments;

    inline Pointer<AstPrintStatement> CloneImpl() const
    {
        return Pointer<AstPrintStatement>(new AstPrintStatement(
            CloneAllAstNodes(m_arguments),
            m_location));
    }
};

#endif
