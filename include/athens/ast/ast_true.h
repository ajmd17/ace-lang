#ifndef AST_TRUE_H
#define AST_TRUE_H

#include <athens/ast/ast_constant.h>

class AstTrue : public AstConstant {
public:
    AstTrue(const SourceLocation &location);

    virtual void Build(AstVisitor *visitor) const;
    virtual int IsTrue() const;
};

#endif