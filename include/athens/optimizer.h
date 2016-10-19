#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <athens/ast_visitor.h>
#include <athens/ast/ast_expression.h>

#include <memory>

class Optimizer : public AstVisitor {
public:
    /** Attemps to reduce a variable that is const literal to the actual value. */
    static void OptimizeExpr(std::shared_ptr<AstExpression> &expr, AstVisitor *visitor);
    
public:
    Optimizer(AstIterator *ast_iterator, CompilationUnit *compilation_unit);
    Optimizer(const Optimizer &other);

    void Optimize();
};

#endif
