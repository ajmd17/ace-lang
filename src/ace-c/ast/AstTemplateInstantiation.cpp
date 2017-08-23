#include <ace-c/ast/AstTemplateInstantiation.hpp>
#include <ace-c/ast/AstAliasDeclaration.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/SemanticAnalyzer.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/my_assert.hpp>
#include <common/utf8.hpp>
#include <common/hasher.hpp>

AstTemplateInstantiation::AstTemplateInstantiation(
    const std::shared_ptr<AstIdentifier> &expr,
    const std::vector<std::shared_ptr<AstArgument>> &generic_args,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_expr(expr),
      m_generic_args(generic_args)
{
}

void AstTemplateInstantiation::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    // visit the expression
    ASSERT(m_expr != nullptr);
    m_expr->Visit(visitor, mod);

    mod->m_scopes.Open(Scope(
        SCOPE_TYPE_NORMAL,
        0
    ));

    // temporarily define all generic parameters.
    if (const Identifier *ident = m_expr->GetProperties().GetIdentifier()) {
        if (ident->GetFlags() & IdentifierFlags::FLAG_GENERIC) {
            const auto &template_params = ident->GetTemplateParams();

            const auto args_substituted = SemanticAnalyzer::Helpers::SubstituteGenericArgs(
                visitor, mod, template_params, m_generic_args, m_location
            );

            // there can be more because of varargs
            if (args_substituted.size() >= template_params.size()) {
                for (size_t i = 0; i < template_params.size(); i++) {
                    // declare aliases for all generic params
                    AstAliasDeclaration(
                        template_params[i].m_name,
                        args_substituted[i],
                        args_substituted[i]->GetLocation()
                    ).Visit(visitor, mod);
                }

                ASSERT(ident->GetCurrentValue() != nullptr);

                m_inst_expr = CloneAstNode(ident->GetCurrentValue());
                m_inst_expr->Visit(visitor, mod);

                std::cout << "inst expr type = " << m_inst_expr->GetExprType()->GetName() << "\n";

                // TODO: Cache instantiations so we don't create a new one for every set of arguments
            }
        } else {
            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                LEVEL_ERROR,
                Msg_expression_not_generic,
                m_expr->GetLocation()
            ));
        }
    }

    mod->m_scopes.Close();
}

std::unique_ptr<Buildable> AstTemplateInstantiation::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    // visit the expression
    ASSERT(m_inst_expr != nullptr);
    return m_inst_expr->Build(visitor, mod);
}

void AstTemplateInstantiation::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    // optimize the expression
    ASSERT(m_inst_expr != nullptr);
    m_inst_expr->Optimize(visitor, mod);
}

Pointer<AstStatement> AstTemplateInstantiation::Clone() const
{
    return CloneImpl();
}

Tribool AstTemplateInstantiation::IsTrue() const
{
    return Tribool::Indeterminate();
}

bool AstTemplateInstantiation::MayHaveSideEffects() const
{
    return true;
}

SymbolTypePtr_t AstTemplateInstantiation::GetExprType() const
{
    if (m_inst_expr != nullptr) {
        return m_inst_expr->GetExprType();
    }

    return BuiltinTypes::UNDEFINED;
}
