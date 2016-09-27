#ifndef AST_DECLARATION_H
#define AST_DECLARATION_H

#include <athens/ast/ast_statement.h>

#include <string>

class AstDeclaration : public AstStatement {
public:
    AstDeclaration(const std::string &name, const SourceLocation &location);
    virtual ~AstDeclaration() = default;

    inline const std::string &GetName() const { return m_name; }

    virtual void Visit(AstVisitor *visitor) = 0;

protected:
    std::string m_name;
};

#endif