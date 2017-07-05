#ifndef AST_HAS_EXPRESSION_HPP
#define AST_HAS_EXPRESSION_HPP

#include <ace-c/ast/AstExpression.hpp>

#include <string>

class AstHasExpression : public AstExpression {
public:
    AstHasExpression(const std::shared_ptr<AstExpression> &target,
      const std::string &field_name,
      const SourceLocation &location);
    virtual ~AstHasExpression() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    
    virtual Pointer<AstStatement> Clone() const override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual SymbolTypePtr_t GetSymbolType() const override;

protected:
    std::shared_ptr<AstExpression> m_target;
    std::string m_field_name;

    // set while analyzing
    int m_has_member;

    inline Pointer<AstHasExpression> CloneImpl() const
    {
        return Pointer<AstHasExpression>(new AstHasExpression(
            CloneAstNode(m_target),
            m_field_name,
            m_location
        ));
    }
};

#endif