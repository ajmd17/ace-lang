#ifndef AST_MIXIN_DECLARATION_HPP
#define AST_MIXIN_DECLARATION_HPP

#include <ace-c/ast/AstDeclaration.hpp>
#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/ast/AstTypeSpecification.hpp>
#include <ace-c/ast/AstVariable.hpp>
#include <ace-c/ast/AstVariableDeclaration.hpp>
#include <ace-c/type-system/SymbolType.hpp>

#include <memory>
#include <string>

class AstMixinDeclaration : public AstDeclaration {
public:
    AstMixinDeclaration(const std::string &name,
        const std::shared_ptr<AstExpression> &expr,
        const SourceLocation &location);
    virtual ~AstMixinDeclaration() = default;

    inline void SetPreventShadowing(bool prevent_shadowing)
        { m_prevent_shadowing = prevent_shadowing; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    
    virtual Pointer<AstStatement> Clone() const override;

protected:
    std::shared_ptr<AstExpression> m_expr;
    //std::string m_mixin_expr;
    bool m_prevent_shadowing;

    // created if there is a shadowed object to allow referencing it
    std::shared_ptr<AstVariableDeclaration> m_shadowed_decl;

    inline Pointer<AstMixinDeclaration> CloneImpl() const
    {
        return Pointer<AstMixinDeclaration>(new AstMixinDeclaration(
            m_name,
            CloneAstNode(m_expr),
            m_location
        ));
    }
};

#endif
