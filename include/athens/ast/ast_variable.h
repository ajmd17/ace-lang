#ifndef AST_VARIABLE_H
#define AST_VARIABLE_H

#include <athens/ast/ast_statement.h>

#include <string>

class AstVariable : public AstStatement {
public:
    AstVariable(const std::string &name, const SourceLocation &location);
    virtual ~AstVariable() = default;

    inline const std::string &GetName() const { return m_name; }

    virtual void Visit(AstVisitor *visitor);

protected:
    std::string m_name;
};

#endif