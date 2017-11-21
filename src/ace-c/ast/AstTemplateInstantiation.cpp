#include <ace-c/ast/AstTemplateInstantiation.hpp>
#include <ace-c/ast/AstTemplateExpression.hpp>
#include <ace-c/ast/AstAliasDeclaration.hpp>
#include <ace-c/ast/AstMixinDeclaration.hpp>
#include <ace-c/ast/AstVariableDeclaration.hpp>
#include <ace-c/ast/AstBlock.hpp>
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
    const std::shared_ptr<AstExpression> &expr,
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
        SCOPE_TYPE_NORMAL, 0
    ));

    for (auto &arg : m_generic_args) {
        ASSERT(arg != nullptr);
        arg->Visit(visitor, mod);
    }

    m_block.reset(new AstBlock({}, m_location));

    if (m_expr->GetExprType() != BuiltinTypes::ANY) {
        // temporarily define all generic parameters.
        const AstExpression *value_of = m_expr->GetValueOf();
        // no need to check if null because if it is the template_expr cast will return null

        m_return_type = BuiltinTypes::ANY;

        if (const AstTemplateExpression *template_expr = dynamic_cast<const AstTemplateExpression*>(value_of)) {
            FunctionTypeSignature_t substituted = SemanticAnalyzer::Helpers::SubstituteFunctionArgs(
                visitor, mod, template_expr->GetExprType(), m_generic_args, m_location
            );

            m_return_type = substituted.first;
            const auto args_substituted = substituted.second;

            // there can be more because of varargs
            if (args_substituted.size() >= template_expr->GetGenericParameters().size()) {
                for (size_t i = 0; i <  template_expr->GetGenericParameters().size(); i++) {
                    ASSERT(args_substituted[i]->GetExpr() != nullptr);

                    if (args_substituted[i]->GetExpr()->GetExprType() != BuiltinTypes::UNDEFINED) {
                        m_block->AddChild(std::shared_ptr<AstVariableDeclaration>(new AstVariableDeclaration(
                            template_expr->GetGenericParameters()[i]->GetName(),
                            nullptr,
                            CloneAstNode(args_substituted[i]->GetExpr()),
                            {},
                            true, // const
                            false, // generic
                            args_substituted[i]->GetLocation()
                        )));
                    }
                }

                ASSERT(template_expr->GetInnerExpression() != nullptr);

                m_block->AddChild(CloneAstNode(template_expr->GetInnerExpression()));
                m_block->Visit(visitor, mod);

                // TODO: Cache instantiations so we don't create a new one for every set of arguments
            }
        } else {
            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                LEVEL_ERROR,
                Msg_expression_not_generic,
                m_expr->GetLocation(),
                m_expr->GetExprType()->GetName()
            ));
        }
    } else {
        m_return_type = BuiltinTypes::ANY;
    }

    mod->m_scopes.Close();
}

std::unique_ptr<Buildable> AstTemplateInstantiation::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    // for (auto &it : m_param_overrides/*m_mixin_overrides*/) {
    //     chunk->Append(it->Build(visitor, mod));
    // }

    // // visit the expression
    // ASSERT(m_inst_expr != nullptr);
    // chunk->Append(m_inst_expr->Build(visitor, mod));

    ASSERT(m_block != nullptr);
    chunk->Append(m_block->Build(visitor, mod));


    // pop stack for all mixin values
    //chunk->Append(Compiler::PopStack(visitor, m_mixin_overrides.size()));

    return std::move(chunk);
}

void AstTemplateInstantiation::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    // for (auto &it : m_param_overrides/*m_mixin_overrides*/) {
    //     it->Optimize(visitor, mod);
    // }

    // // optimize the expression
    // ASSERT(m_inst_expr != nullptr);
    // m_inst_expr->Optimize(visitor, mod);

    ASSERT(m_block != nullptr);
    m_block->Optimize(visitor, mod);
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
    ASSERT(m_return_type != nullptr);
    return m_return_type;
}
