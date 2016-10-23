#ifndef AST_DECLARATION_HPP
#define AST_DECLARATION_HPP

#include <athens/ast/ast_statement.hpp>
#include <athens/identifier.hpp>

#include <string>

class AstDeclaration : public AstStatement {
public:
    AstDeclaration(const std::string &name, const SourceLocation &location);
    virtual ~AstDeclaration() = default;

    inline const std::string &GetName() const { return m_name; }
    inline Identifier *GetIdentifier() const { return m_identifier; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override = 0;

protected:
    std::string m_name;
    Identifier *m_identifier;
};

#endif
