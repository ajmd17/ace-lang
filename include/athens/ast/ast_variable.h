#ifndef AST_VARIABLE_H
#define AST_VARIABLE_H

#include <athens/ast/ast_statement.h>
#include <athens/identifier.h>

#include <string>

class AstVariable : public AstExpression {
public:
    AstVariable(const std::string &name, const SourceLocation &location);
    virtual ~AstVariable() = default;

    inline const std::string &GetName() const { return m_name; }
    inline Identifier *GetIdentifier() const { return m_identifier; }

    virtual void Visit(AstVisitor *visitor);
    virtual void Build(AstVisitor *visitor) const;
    virtual void Optimize(AstVisitor *visitor);

    virtual int IsTrue() const;

protected:
    std::string m_name;
    Identifier *m_identifier;
};

#endif