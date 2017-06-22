#ifndef AST_PRINT_STATEMENT_HPP
#define AST_PRINT_STATEMENT_HPP

#include <ace-c/ast/AstStatement.hpp>
#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/ast/AstArgumentList.hpp>

#include <vector>
#include <memory>

class AstPrintStatement : public AstStatement {
public:
    AstPrintStatement(const std::shared_ptr<AstArgumentList> &arg_list,
        const SourceLocation &location);
    virtual ~AstPrintStatement() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;
    virtual Pointer<AstStatement> Clone() const override;

private:
    std::shared_ptr<AstArgumentList> m_arg_list;

    inline Pointer<AstPrintStatement> CloneImpl() const
    {
        return Pointer<AstPrintStatement>(new AstPrintStatement(
            CloneAstNode(m_arg_list),
            m_location
        ));
    }
};

#endif
