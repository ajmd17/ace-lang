#include <ace-c/ast/ast_module_access.hpp>
#include <ace-c/emit/instruction.hpp>
#include <ace-c/emit/static_object.hpp>
#include <ace-c/ast/ast_variable.hpp>
#include <ace-c/ast/ast_function_call.hpp>
#include <ace-c/ast/ast_generated_expression.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/compiler.hpp>
#include <ace-c/module.hpp>
#include <ace-c/configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/hasher.hpp>

AstModuleAccess::AstModuleAccess(const std::string &target,
    const std::shared_ptr<AstExpression> &expr,
    const SourceLocation &location)
    : AstExpression(location),
      m_target(target),
      m_expr(expr),
      m_mod_access(nullptr)
{
}

void AstModuleAccess::Visit(AstVisitor *visitor, Module *mod)
{
    // check all modules for one with the same name
    for (int i = 0; i < visitor->GetCompilationUnit()->m_modules.size(); i++) {
        auto &current = visitor->GetCompilationUnit()->m_modules[i];
        if (current != nullptr && current->GetName() == m_target) {
            // module with name found
            m_mod_access = current.get();
            break;
        }
    }

    if (m_mod_access) {
        m_expr->Visit(visitor, m_mod_access);
    } else {
        CompilerError err(Level_fatal, Msg_unknown_module, m_location, m_target);
        visitor->GetCompilationUnit()->GetErrorList().AddError(err);
    }
}

void AstModuleAccess::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_mod_access != nullptr);
    m_expr->Build(visitor, m_mod_access);
}

void AstModuleAccess::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_mod_access != nullptr);
    m_expr->Optimize(visitor, m_mod_access);
}

int AstModuleAccess::IsTrue() const
{
    return m_expr->IsTrue();
}

bool AstModuleAccess::MayHaveSideEffects() const
{
    return m_expr->MayHaveSideEffects();
}

ObjectType AstModuleAccess::GetObjectType() const
{
    return m_expr->GetObjectType();
}
