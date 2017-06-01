#ifndef AST_CLOSURE_HPP
#define AST_CLOSURE_HPP

#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/ast/AstFunctionExpression.hpp>

#include <string>
#include <vector>
#include <memory>

class AstClosure : public AstExpression {
public:
    AstClosure(const std::shared_ptr<AstFunctionExpression> &expr,
        const SourceLocation &location);
    virtual ~AstClosure() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;
    virtual Pointer<AstStatement> Clone() const override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual SymbolTypePtr_t GetSymbolType() const override;

private:
    std::shared_ptr<AstFunctionExpression> m_expr;

    inline std::shared_ptr<AstClosure> CloneImpl() const
    {
        return std::shared_ptr<AstClosure>(new AstClosure(
            CloneAstNode(m_expr),
            m_location
        ));
    }
};

#endif