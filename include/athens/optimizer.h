#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <athens/ast_visitor.h>

class Optimizer : public AstVisitor {
public:
    Optimizer(AstIterator *ast_iterator, CompilationUnit *compilation_unit);
    Optimizer(const Optimizer &other);

    void Optimize();
};

#endif