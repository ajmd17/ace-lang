#ifndef AST_IDENTIFIER_HPP
#define AST_IDENTIFIER_HPP

#include <athens/ast/ast_expression.hpp>
#include <athens/identifier.hpp>

#include <string>

class AstIdentifier : public AstExpression {
public:
    AstIdentifier(const std::string &name, const SourceLocation &location);
    virtual ~AstIdentifier() = default;

    inline const std::string &GetName() const { return m_name; }
    inline Identifier *GetIdentifier() const { return m_identifier; }

    virtual void Visit(AstVisitor *visitor) override;
    virtual void Build(AstVisitor *visitor) override = 0;
    virtual void Optimize(AstVisitor *visitor) override = 0;
    virtual int IsTrue() const override = 0;
    virtual bool MayHaveSideEffects() const override = 0;

protected:
    std::string m_name;
    Identifier *m_identifier;
};

#endif
