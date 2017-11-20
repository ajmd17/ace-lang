#include <ace-c/ast/AstSymbolQuery.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Configuration.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/my_assert.hpp>

AstSymbolQuery::AstSymbolQuery(
    const std::string &command_name,
    const std::shared_ptr<AstExpression> &expr,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_command_name(command_name),
      m_expr(expr)
{
}

void AstSymbolQuery::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);
    m_expr->Visit(visitor, mod);

    m_symbol_type = BuiltinTypes::UNDEFINED;

    if (m_command_name == "inspect_type") {
        SymbolTypePtr_t expr_type = m_expr->GetExprType();
        ASSERT(expr_type != nullptr);

        m_string_result_value = std::shared_ptr<AstString>(new AstString(
            expr_type->GetName(),
            m_location
        ));
    } else {
        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
            LEVEL_ERROR,
            Msg_invalid_symbol_query,
            m_location,
            m_command_name
        ));
    }
}

std::unique_ptr<Buildable> AstSymbolQuery::Build(AstVisitor *visitor, Module *mod)
{
    if (AstExpression *value_of = const_cast<AstExpression*>(GetValueOf())) {
        return value_of->Build(visitor, mod);
    }

    return nullptr;
}

void AstSymbolQuery::Optimize(AstVisitor *visitor, Module *mod)
{
}

Pointer<AstStatement> AstSymbolQuery::Clone() const
{
    return CloneImpl();
}

Tribool AstSymbolQuery::IsTrue() const
{
    return Tribool::Indeterminate();
}

bool AstSymbolQuery::MayHaveSideEffects() const
{
    return false;
}

SymbolTypePtr_t AstSymbolQuery::GetExprType() const
{
    if (m_string_result_value != nullptr) {
        return BuiltinTypes::STRING;
    }

    return BuiltinTypes::UNDEFINED;
}

const AstExpression *AstSymbolQuery::GetValueOf() const
{
    if (m_string_result_value != nullptr) {
        return m_string_result_value.get();
    }

    return nullptr;
}