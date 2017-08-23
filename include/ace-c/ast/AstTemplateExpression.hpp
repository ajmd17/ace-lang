#ifndef AST_TEMPLATE_EXPRESSION_HPP
#define AST_TEMPLATE_EXPRESSION_HPP

#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/ast/AstParameter.hpp>
#include <ace-c/type-system/SymbolType.hpp>

#include <string>

class AstTemplateExpression : public AstExpression {
public:
    AstTemplateExpression(const std::shared_ptr<AstExpression> &expr,
        const std::vector<std::shared_ptr<AstParameter>> &generic_params,
        const SourceLocation &location);
    virtual ~AstTemplateExpression() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    
    virtual Pointer<AstStatement> Clone() const override;

    virtual Tribool IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual SymbolTypePtr_t GetExprType() const override;

private:
    std::shared_ptr<AstExpression> m_expr;
    std::vector<std::shared_ptr<AstParameter>> m_generic_params;

    inline Pointer<AstTemplateExpression> CloneImpl() const
    {
        return Pointer<AstTemplateExpression>(new AstTemplateExpression(
            CloneAstNode(m_expr),
            CloneAllAstNodes(m_generic_params),
            m_location
        ));
    }
};

#endif