#ifndef AST_EXPRESSION_H
#define AST_EXPRESSION_H

#include <athens/ast/ast_statement.h>

class AstExpression : public AstStatement {
public:
    AstExpression(const SourceLocation &location);
    virtual ~AstExpression() = default;

    virtual void Visit(AstVisitor *visitor) = 0;
    virtual void Build(AstVisitor *visitor) = 0;
    virtual void Optimize(AstVisitor *visitor) = 0;

    /** Determines whether the expression would evaluate to true. 
        Return -1 if it cannot be evaluated at compile time. 
    */
    virtual int IsTrue() const = 0;
    
    inline int IsFalse() const
    { 
        int is_true = IsTrue();
        return (is_true == -1) ? is_true : !is_true;
    }

    bool m_is_standalone;
};

#endif