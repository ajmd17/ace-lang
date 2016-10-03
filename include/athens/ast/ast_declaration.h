#ifndef AST_DECLARATION_H
#define AST_DECLARATION_H

#include <athens/ast/ast_statement.h>
#include <athens/identifier.h>

#include <string>

class AstDeclaration : public AstStatement {
public:
    AstDeclaration(const std::string &name, const SourceLocation &location);
    virtual ~AstDeclaration() = default;

    inline const std::string &GetName() const { return m_name; }
    inline Identifier *GetIdentifier() const { return m_identifier; }

    virtual void Visit(AstVisitor *visitor);
    virtual void Build(AstVisitor *visitor) const = 0;
    virtual void Optimize(AstVisitor *visitor) = 0;

protected:
    std::string m_name;
    Identifier *m_identifier;
};

#endif