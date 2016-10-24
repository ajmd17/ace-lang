#include <athens/ast/ast_float.hpp>
#include <athens/ast/ast_integer.hpp>
#include <athens/ast/ast_true.hpp>
#include <athens/ast/ast_false.hpp>
#include <athens/ast/ast_undefined.hpp>
#include <athens/ast/ast_null.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/emit/instruction.hpp>

#include <common/instructions.hpp>

#include <limits>
#include <cmath>

AstFloat::AstFloat(a_float value, const SourceLocation &location)
    : AstConstant(location),
      m_value(value)
{
}

void AstFloat::Build(AstVisitor *visitor, Module *mod)
{
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // load integer value into register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, float>(LOAD_F32, rp, m_value);
}

int AstFloat::IsTrue() const
{
    // any non-zero value is considered true
    return m_value != 0.0f;
}

ObjectType AstFloat::GetObjectType() const
{
    return ObjectType::type_builtin_float;
}

bool AstFloat::IsNumber() const
{
    return true;
}

a_int AstFloat::IntValue() const
{
    return static_cast<a_int>(m_value);
}

a_float AstFloat::FloatValue() const
{
    return m_value;
}

std::shared_ptr<AstConstant> AstFloat::operator+(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstFloat>(
        new AstFloat(FloatValue() + right->FloatValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator-(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstFloat>(
        new AstFloat(FloatValue() - right->FloatValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator*(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstFloat>(
        new AstFloat(FloatValue() * right->FloatValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator/(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    a_float right_float = right->FloatValue();
    if (right_float == 0.0) {
        // division by zero
        return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
    }
    return std::shared_ptr<AstFloat>(new AstFloat(FloatValue() / right_float, m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator%(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    a_float right_float = right->FloatValue();
    if (right_float == 0.0) {
        // division by zero
        return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
    }

    return std::shared_ptr<AstFloat>(new AstFloat(std::fmod(FloatValue(), right_float), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator^(AstConstant *right) const
{
    // invalid operator on floats
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator&(AstConstant *right) const
{
    // invalid operator on floats
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator|(AstConstant *right) const
{
    // invalid operator on floats
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator<<(AstConstant *right) const
{
    // invalid operator on floats
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator>>(AstConstant *right) const
{
    // invalid operator on floats
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator&&(AstConstant *right) const
{
    int this_true = IsTrue();
    int right_true = right->IsTrue();

    if (!right->IsNumber()) {
        // this operator is valid to compare against null
        AstNull *ast_null = dynamic_cast<AstNull*>(right);
        if (ast_null != nullptr) {
            // rhs is null, return false
            return std::shared_ptr<AstFalse>(
                new AstFalse(m_location));
        }
        return nullptr;
    }

    if (this_true == 1 && right_true == 1) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    } else if (this_true == 0 && right_true == 0) {
        return std::shared_ptr<AstFalse>(new AstFalse(m_location));
    } else {
        // indeterminate
        return nullptr;
    }
}

std::shared_ptr<AstConstant> AstFloat::operator||(AstConstant *right) const
{
    int this_true = IsTrue();
    int right_true = right->IsTrue();

    if (!right->IsNumber()) {
        // this operator is valid to compare against null
        AstNull *ast_null = dynamic_cast<AstNull*>(right);
        if (ast_null != nullptr) {
            if (this_true == 1) {
                return std::shared_ptr<AstTrue>(new AstTrue(m_location));
            } else if (this_true == 0) {
                return std::shared_ptr<AstFalse>(new AstFalse(m_location));
            }
        }
        return nullptr;
    }

    if (this_true == 1 || right_true == 1) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    } else if (this_true == 0 || right_true == 0) {
        return std::shared_ptr<AstFalse>(new AstFalse(m_location));
    } else {
        // indeterminate
        return nullptr;
    }
}

std::shared_ptr<AstConstant> AstFloat::Equals(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstFloat>(
        new AstFloat(FloatValue() == right->FloatValue(), m_location));
}
