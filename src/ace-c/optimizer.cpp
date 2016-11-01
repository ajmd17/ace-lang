#include <ace-c/optimizer.hpp>
#include <ace-c/ast/ast_module_declaration.hpp>
#include <ace-c/ast/ast_binary_expression.hpp>
#include <ace-c/ast/ast_variable.hpp>
#include <ace-c/ast/ast_constant.hpp>

void Optimizer::OptimizeExpr(std::shared_ptr<AstExpression> &expr, AstVisitor *visitor, Module *mod)
{
    expr->Optimize(visitor, mod);

    AstVariable *expr_as_var = nullptr;
    AstBinaryExpression *expr_as_binop = nullptr;
    if ((expr_as_var = dynamic_cast<AstVariable*>(expr.get())) != nullptr) {
        // the side is a variable, so we can further optimize by inlining,
        // only if it is const, and a literal.
        if (expr_as_var->GetIdentifier() != nullptr) {
            if (expr_as_var->GetIdentifier()->GetFlags() & FLAG_CONST) {
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
    if (m_ast_iterator->HasNext()) {
        auto first_statement = m_ast_iterator->Next();
        auto module_declaration = std::dynamic_pointer_cast<AstModuleDeclaration>(first_statement);

        if (module_declaration != nullptr) {
            // all files must begin with a module declaration
            module_declaration->Optimize(this, nullptr);
            m_compilation_unit->m_module_index++;

            Module *mod = m_compilation_unit->m_modules[m_compilation_unit->m_module_index].get();

            while (m_ast_iterator->HasNext()) {
                m_ast_iterator->Next()->Optimize(this, mod);
            }

            // decrement the index to refer to the previous module
            m_compilation_unit->m_module_index--;
        }
    }
}
