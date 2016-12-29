#ifndef OPTIMIZER_HPP
#define OPTIMIZER_HPP

#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/ast/AstExpression.hpp>

#include <memory>

class Optimizer : public AstVisitor {
public:
    /** Attemps to reduce a variable that is const literal to the actual value. */
    static void OptimizeExpr(std::shared_ptr<AstExpression> &expr,
        AstVisitor *visitor, Module *mod);

public:
    Optimizer(AstIterator *ast_iterator, CompilationUnit *compilation_unit);
    Optimizer(const Optimizer &other);

    void Optimize(bool expect_module_decl = true);

private:
    void OptimizeInner();
};

#endif
