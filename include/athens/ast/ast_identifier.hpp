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

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override = 0;

    virtual int IsTrue() const override = 0;
    virtual bool MayHaveSideEffects() const override = 0;
    virtual ObjectType GetObjectType() const override;

protected:
    std::string m_name;
    Identifier *m_identifier;

    int GetStackOffset(int stack_size) const;
};

#endif
