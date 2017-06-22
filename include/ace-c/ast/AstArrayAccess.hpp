#ifndef AST_ARRAY_ACCESS_HPP
#define AST_ARRAY_ACCESS_HPP

#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/enums.hpp>

#include <string>
#include <vector>
#include <memory>

class AstArrayAccess : public AstExpression {
public:
    AstArrayAccess(const std::shared_ptr<AstExpression> &target,
        const std::shared_ptr<AstExpression> &index,
        const SourceLocation &location);
    virtual ~AstArrayAccess() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;
    virtual Pointer<AstStatement> Clone() const override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;
    virtual SymbolTypePtr_t GetSymbolType() const override;

private:
    std::shared_ptr<AstExpression> m_target;
    std::shared_ptr<AstExpression> m_index;

    inline std::shared_ptr<AstArrayAccess> CloneImpl() const
    {
        return std::shared_ptr<AstArrayAccess>(new AstArrayAccess(
            CloneAstNode(m_target),
            CloneAstNode(m_index),
            m_location
        ));
    }
};

#endif
