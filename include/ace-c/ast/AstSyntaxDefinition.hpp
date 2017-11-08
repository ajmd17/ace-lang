#ifndef AST_SYNTAX_DEFINITION_HPP
#define AST_SYNTAX_DEFINITION_HPP

#include <ace-c/ast/AstStatement.hpp>
#include <ace-c/ast/AstString.hpp>

#include <memory>
#include <vector>

class AstSyntaxDefinition : public AstStatement {
public:
    AstSyntaxDefinition(
        const std::shared_ptr<AstString> &syntax_string,
        const std::shared_ptr<AstString> &transform_string,
        const SourceLocation &location);
    virtual ~AstSyntaxDefinition() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    
    virtual Pointer<AstStatement> Clone() const override;

protected:
    std::shared_ptr<AstString> m_syntax_string;
    std::shared_ptr<AstString> m_transform_string;

    inline Pointer<AstSyntaxDefinition> CloneImpl() const
    {
        return Pointer<AstSyntaxDefinition>(new AstSyntaxDefinition(
            CloneAstNode(m_syntax_string),
            CloneAstNode(m_transform_string),
            m_location
        ));
    }
};

#endif
