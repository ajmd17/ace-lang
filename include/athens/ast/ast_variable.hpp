#ifndef AST_VARIABLE_HPP
#define AST_VARIABLE_HPP

#include <athens/ast/ast_statement.hpp>
#include <athens/identifier.hpp>

#include <string>

class AstVariable : public AstExpression {
public:
    AstVariable(const std::string &name, const SourceLocation &location);
    virtual ~AstVariable() = default;

    inline const std::string &GetName() const { return m_name; }
    inline Identifier *GetIdentifier() const { return m_identifier; }

    virtual void Visit(AstVisitor *visitor) override;
    virtual void Build(AstVisitor *visitor) override;
    virtual void Optimize(AstVisitor *visitor) override;

    virtual int IsTrue() const override;
    virtual bool MayHaveSideEffects() const override;

protected:
    std::string m_name;
    Identifier *m_identifier;
};

#endif
