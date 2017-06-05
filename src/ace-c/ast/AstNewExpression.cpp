#include <ace-c/ast/AstNewExpression.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>

#include <common/my_assert.hpp>
#include <common/utf8.hpp>

AstNewExpression::AstNewExpression(
    const std::shared_ptr<AstTypeSpecification> &type_expr,
    const std::shared_ptr<AstArgumentList> &arg_list,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_type_expr(type_expr),
      m_arg_list(arg_list)
{
}

void AstNewExpression::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_type_expr != nullptr);
    m_type_expr->Visit(visitor, mod);

    if (m_arg_list != nullptr) {
        m_arg_list->Visit(visitor, mod);
    }

    SymbolTypePtr_t object_type = m_type_expr->GetSymbolType();
    ASSERT(object_type != nullptr);
    // get default value
    auto object_value = object_type->GetDefaultValue();
    ASSERT(object_value != nullptr);

    m_object_value = object_value;
}

void AstNewExpression::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_type_expr != nullptr);
    m_type_expr->Build(visitor, mod);

    if (m_arg_list != nullptr) {
        m_arg_list->Build(visitor, mod);
    }

    // build in the value
    ASSERT(m_object_value != nullptr);
    m_object_value->Build(visitor, mod);
}

void AstNewExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_type_expr != nullptr);
    m_type_expr->Optimize(visitor, mod);

    if (m_arg_list != nullptr) {
        m_arg_list->Optimize(visitor, mod);
    }

    // build in the value
    ASSERT(m_object_value != nullptr);
    m_object_value->Optimize(visitor, mod);
}

void AstNewExpression::Recreate(std::ostringstream &ss)
{
    ASSERT(m_type_expr != nullptr);

    ss << "new ";
    m_type_expr->Recreate(ss);
    ss << "(";
    if (m_arg_list != nullptr) {
        m_arg_list->Recreate(ss);
    }
    ss << ")";
}

Pointer<AstStatement> AstNewExpression::Clone() const
{
    return CloneImpl();
}

int AstNewExpression::IsTrue() const
{
    ASSERT(m_object_value != nullptr);
    return m_object_value->IsTrue();
}

bool AstNewExpression::MayHaveSideEffects() const
{
    return true;
}

SymbolTypePtr_t AstNewExpression::GetSymbolType() const
{
    ASSERT(m_type_expr != nullptr);
    ASSERT(m_type_expr->GetSymbolType() != nullptr);
    return m_type_expr->GetSymbolType();
}
