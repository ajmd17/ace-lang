#include <ace-c/ast/AstTemplateExpression.hpp>
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

AstTemplateExpression::AstTemplateExpression(
    const std::shared_ptr<AstExpression> &expr,
    const std::vector<std::shared_ptr<AstParameter>> &generic_params,
    const std::shared_ptr<AstPrototypeSpecification> &return_type_specification,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_expr(expr),
      m_generic_params(generic_params),
      m_return_type_specification(return_type_specification)
{
}

void AstTemplateExpression::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    // visit params before expression to make declarations
    // for things that may be used in the expression
    for (auto &generic_param : m_generic_params) {
        ASSERT(generic_param != nullptr);
        generic_param->Visit(visitor, mod);
    }

    // added because i made creation of this object happen in parser
    ASSERT(m_expr != nullptr);
    m_expr->Visit(visitor, mod);

    ASSERT(m_expr->GetExprType() != nullptr);

    std::vector<GenericInstanceTypeInfo::Arg> generic_param_types;
    generic_param_types.reserve(m_generic_params.size() + 1);
    
    SymbolTypePtr_t expr_return_type;

    // if return type has been specified - visit it and check to see
    // if it's compatible with the expression
    if (m_return_type_specification != nullptr) {
        m_return_type_specification->Visit(visitor, mod);

        ASSERT(m_return_type_specification->GetHeldType() != nullptr);

        SemanticAnalyzer::Helpers::EnsureLooseTypeAssignmentCompatibility(
            visitor,
            mod,
            m_return_type_specification->GetHeldType(),
            m_expr->GetExprType(),
            m_return_type_specification->GetLocation()
        );

        expr_return_type = m_return_type_specification->GetHeldType();
    } else {
        expr_return_type = m_expr->GetExprType();
    }

    generic_param_types.push_back({
        "@return",
        expr_return_type
    });

    for (size_t i = 0; i < m_generic_params.size(); i++) {
        const auto &param = m_generic_params[i];

        ASSERT(param != nullptr);

        generic_param_types.push_back(GenericInstanceTypeInfo::Arg {
            param->GetName(),
            param->GetIdentifier()->GetSymbolType(),
            nullptr
        });
    }

    m_symbol_type = SymbolType::GenericInstance(
        BuiltinTypes::GENERIC_VARIABLE_TYPE,
        GenericInstanceTypeInfo {
            generic_param_types
        }
    );
}

std::unique_ptr<Buildable> AstTemplateExpression::Build(AstVisitor *visitor, Module *mod)
{
    // attempt at using the template expression directly...
    return nullptr;
}

void AstTemplateExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);
    m_expr->Optimize(visitor, mod);
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
    // ASSERT(m_expr != nullptr);
    // return m_expr->GetExprType();
    ASSERT(m_symbol_type != nullptr);
    return m_symbol_type;
}
