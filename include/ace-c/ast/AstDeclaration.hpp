#ifndef AST_DECLARATION_HPP
#define AST_DECLARATION_HPP

#include <ace-c/ast/AstStatement.hpp>
#include <ace-c/Identifier.hpp>

#include <string>

class AstDeclaration : public AstStatement {
public:
    AstDeclaration(const std::string &name,
        const SourceLocation &location);
    virtual ~AstDeclaration() = default;

    inline const std::string &GetName() const { return m_name; }
    inline Identifier *GetIdentifier() const { return m_identifier; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Recreate(std::ostringstream &ss) override = 0;
    virtual Pointer<AstStatement> Clone() const override = 0;

protected:
    std::string m_name;
    Identifier *m_identifier;
};

#endif
