#ifndef AST_FALSE_H
#define AST_FALSE_H

#include <athens/ast/ast_constant.h>

class AstFalse : public AstConstant {
public:
    AstFalse(const SourceLocation &location);

    virtual void Build(AstVisitor *visitor) const;
    virtual int IsTrue() const;
};

#endif