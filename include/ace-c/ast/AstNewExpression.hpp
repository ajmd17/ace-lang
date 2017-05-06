#ifndef AST_NEW_EXPRESSION_HPP
#define AST_NEW_EXPRESSION_HPP

#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/ast/AstArgument.hpp>
#include <ace-c/ast/AstTypeSpecification.hpp>
#include <ace-c/SymbolType.hpp>

#include <string>
#include <memory>

class AstNewExpression: public AstExpression {
public:
    AstNewExpression(
        const std::shared_ptr<AstTypeSpecification> &type_expr,
        const std::vector<std::shared_ptr<AstArgument>> &args,
        const SourceLocation &location);
    virtual ~AstNewExpression() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;
    virtual Pointer<AstStatement> Clone() const override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual SymbolTypePtr_t GetSymbolType() const override;

private:
    std::shared_ptr<AstTypeSpecification> m_type_expr;
    std::vector<std::shared_ptr<AstArgument>> m_args;

    /** Set while analyzing */
    std::shared_ptr<AstExpression> m_object_value;

    inline Pointer<AstNewExpression> CloneImpl() const
    {
        return Pointer<AstNewExpression>(new AstNewExpression(
            CloneAstNode(m_type_expr),
            CloneAllAstNodes(m_args),
            m_location
        ));
    }
};

#endif
