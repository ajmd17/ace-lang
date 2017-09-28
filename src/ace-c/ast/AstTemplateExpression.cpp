#include <ace-c/ast/AstTemplateExpression.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Compiler.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/my_assert.hpp>
#include <common/utf8.hpp>
#include <common/hasher.hpp>

AstTemplateExpression::AstTemplateExpression(
    const std::shared_ptr<AstExpression> &expr,
    const std::vector<std::shared_ptr<AstParameter>> &generic_params,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_expr(expr),
      m_generic_params(generic_params)
{
}

void AstTemplateExpression::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    std::string signature;

    for (size_t i = 0; i < m_generic_params.size(); i++) {
        const auto &param = m_generic_params[i];

        ASSERT(param != nullptr);
        //ASSERT(param->GetIdentifier() != nullptr);
        //ASSERT(param->GetIdentifier()->GetSymbolType() != nullptr);

        signature.append(param->GetName());
        //signature.append(" : ");
        //signature.append(param->GetIdentifier()->GetSymbolType()->GetName());

        if (i != m_generic_params.size() - 1) {
            signature.append(", ");
        }
    }

    visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
        LEVEL_ERROR,
        Msg_generic_expression_invalid_arguments,
        m_location,
        signature
    ));

   /* // temporarily define all generic parameters.
    mod->m_scopes.Open(Scope());

    for (auto &param : m_generic_params) {
        ASSERT(param != nullptr);
        // add the identifier to the table
        param->Visit(visitor, mod);
    }

    // now, visit the expression
    ASSERT(m_expr != nullptr);
    m_expr->Visit(visitor, mod);

    mod->m_scopes.Close();*/
}

std::unique_ptr<Buildable> AstTemplateExpression::Build(AstVisitor *visitor, Module *mod)
{
    return nullptr;
}

void AstTemplateExpression::Optimize(AstVisitor *visitor, Module *mod)
{
}

Pointer<AstStatement> AstTemplateExpression::Clone() const
{
    return CloneImpl();
}

Tribool AstTemplateExpression::IsTrue() const
{
    return Tribool::Indeterminate();
}

bool AstTemplateExpression::MayHaveSideEffects() const
{
    return true;
}

SymbolTypePtr_t AstTemplateExpression::GetExprType() const
{
    ASSERT(m_expr != nullptr);
    return m_expr->GetExprType();
}
