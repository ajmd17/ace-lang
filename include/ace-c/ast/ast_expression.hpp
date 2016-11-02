#ifndef AST_EXPRESSION_HPP
#define AST_EXPRESSION_HPP

#include <ace-c/ast/ast_statement.hpp>
#include <ace-c/object_type.hpp>

class AstExpression : public AstStatement {
public:
    AstExpression(const SourceLocation &location);
    virtual ~AstExpression() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Build(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override = 0;

    /** Determine whether the expression would evaluate to true.
        Returns -1 if it cannot be evaluated at compile time.
    */
    virtual int IsTrue() const = 0;
    inline int IsFalse() const { int t = IsTrue(); return (t == -1) ? t : !t; }
    /** Determine whether or not there is a possibility of side effects. */
    virtual bool MayHaveSideEffects() const = 0;
    virtual ObjectType GetObjectType() const = 0;

    bool m_is_standalone;
};

#endif