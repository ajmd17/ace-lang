#ifndef AST_TYPE_ALIAS_HPP
#define AST_TYPE_ALIAS_HPP

#include <ace-c/ast/AstStatement.hpp>
#include <ace-c/ast/AstTypeSpecification.hpp>
#include <ace-c/SymbolType.hpp>

#include <string>
#include <memory>

class AstTypeAlias: public AstStatement {
public:
    AstTypeAlias(
        const std::string &name,
        const std::shared_ptr<AstTypeSpecification> &aliasee,
        const SourceLocation &location);
    virtual ~AstTypeAlias() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    
    virtual Pointer<AstStatement> Clone() const override;

private:
    std::string m_name;
    std::shared_ptr<AstTypeSpecification> m_aliasee;

    inline Pointer<AstTypeAlias> CloneImpl() const
    {
        return Pointer<AstTypeAlias>(new AstTypeAlias(
            m_name,
            CloneAstNode(m_aliasee),
            m_location
        ));
    }
};

#endif