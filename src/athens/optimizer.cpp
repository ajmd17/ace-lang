#include <athens/optimizer.h>
#include <athens/ast/ast_binary_expression.h>
#include <athens/ast/ast_variable.h>
#include <athens/ast/ast_constant.h>

void Optimizer::OptimizeExpr(std::shared_ptr<AstExpression> &expr, AstVisitor *visitor)
{
    expr->Optimize(visitor);

    AstVariable *expr_as_var = nullptr;
    AstBinaryExpression *expr_as_binop = nullptr;
    if ((expr_as_var = dynamic_cast<AstVariable*>(expr.get())) != nullptr) {
        // the side is a variable, so we can further optimize by inlining,
        // only if it is const, and a literal.
        if (expr_as_var->GetIdentifier() != nullptr) {
            if (expr_as_var->GetIdentifier()->GetFlags() & (Flag_const)) {
                // the variable is a const, now we make sure that the current
                // value is a literal value
                auto value_sp = expr_as_var->GetIdentifier()->GetCurrentValue().lock();
                AstConstant *constant_sp = dynamic_cast<AstConstant*>(value_sp.get());
                if (constant_sp != nullptr) {
                    // yay! we were able to retrieve the value that
                    // the variable is set to, so now we can use that
                    // at compile-time rather than using a variable.
                    expr.reset(constant_sp);
                }
            }
        }
    } else if ((expr_as_binop = dynamic_cast<AstBinaryExpression*>(expr.get())) != nullptr) {
        if (expr_as_binop->GetRight() == nullptr) {
            // right side has been optimized away, to just left side
            expr = expr_as_binop->GetLeft();
        }
    }
}

Optimizer::Optimizer(AstIterator *ast_iterator, CompilationUnit *compilation_unit)
    : AstVisitor(ast_iterator, compilation_unit)
{
}

Optimizer::Optimizer(const Optimizer &other)
    : AstVisitor(other.m_ast_iterator, other.m_compilation_unit)
{
}

void Optimizer::Optimize()
{
    while (m_ast_iterator->HasNext()) {
        m_ast_iterator->Next()->Optimize(this);
    }
}
