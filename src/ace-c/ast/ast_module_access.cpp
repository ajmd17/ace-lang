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
      m_mod_access(nullptr),
      m_is_chained(false),
      m_looked_up(false)
{
}

void AstModuleAccess::PerformLookup(AstVisitor *visitor, Module *mod)
{
    if (m_is_chained) {
        ASSERT(mod != nullptr);
        ASSERT(mod->GetImportTreeLink() != nullptr);

        // search siblings of the current module,
        // rather than global lookup.
        for (auto *sibling : mod->GetImportTreeLink()->m_siblings) {
            ASSERT(sibling != nullptr);
            ASSERT(sibling->m_value != nullptr);
            
            if (sibling->m_value->GetName() == m_target) {
                m_mod_access = sibling->m_value;
            }
        }
    } else {
        m_mod_access = visitor->GetCompilationUnit()->LookupModule(m_target);
    }

    m_looked_up = true;
}

void AstModuleAccess::Visit(AstVisitor *visitor, Module *mod)
{
    if (!m_looked_up) {
        PerformLookup(visitor, mod);
    }

    if (AstModuleAccess *expr_mod_access = dynamic_cast<AstModuleAccess*>(m_expr.get())) {
        // set expr to be chained
        expr_mod_access->m_is_chained = true;
    }

    // check modules for one with the same name
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

void AstModuleAccess::Recreate(std::ostringstream &ss)
{
    ASSERT(m_expr != nullptr);
    ss << m_target << "::";
    m_expr->Recreate(ss);
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
