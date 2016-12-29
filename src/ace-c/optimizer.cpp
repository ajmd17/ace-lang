#include <ace-c/Optimizer.hpp>
#include <ace-c/ast/AstModuleDeclaration.hpp>
#include <ace-c/ast/AstBinaryExpression.hpp>
#include <ace-c/ast/AstVariable.hpp>
#include <ace-c/ast/AstConstant.hpp>

#include <common/my_assert.hpp>

void Optimizer::OptimizeExpr(std::shared_ptr<AstExpression> &expr, AstVisitor *visitor, Module *mod)
{
    ASSERT(expr != nullptr);
    expr->Optimize(visitor, mod);

    if (auto *expr_as_var = dynamic_cast<AstVariable*>(expr.get())) {
        // the side is a variable, so we can further optimize by inlining,
        // only if it is const, and a literal.
        if (expr_as_var->GetProperties().GetIdentifier()) {
            if (expr_as_var->GetProperties().GetIdentifier()->GetFlags() & FLAG_CONST) {
                // the variable is a const, now we make sure that the current
                // value is a literal value
                if (auto *constant = dynamic_cast<AstConstant*>(expr_as_var->GetProperties().GetIdentifier()->GetCurrentValue().get())) {
                    // yay! we were able to retrieve the value that
                    // the variable is set to, so now we can use that
                    // at compile-time rather than using a variable.
                    expr.reset(constant);
                }
            }
        }
    } else if (auto *expr_as_binop = dynamic_cast<AstBinaryExpression*>(expr.get())) {
        if (!expr_as_binop->GetRight()) {
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

void Optimizer::Optimize(bool expect_module_decl)
{
    if (expect_module_decl) {
        if (m_ast_iterator->HasNext()) {
            auto first_statement = m_ast_iterator->Next();
            auto module_declaration = std::dynamic_pointer_cast<AstModuleDeclaration>(first_statement);

            if (module_declaration) {
                // all files must begin with a module declaration
                module_declaration->Optimize(this, nullptr);
                OptimizeInner();
            }
        }
    } else {
        OptimizeInner();
    }
}

void Optimizer::OptimizeInner()
{
    Module *mod = m_compilation_unit->GetCurrentModule();
    while (m_ast_iterator->HasNext()) {
        m_ast_iterator->Next()->Optimize(this, mod);
    }
}
