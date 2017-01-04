#include <ace-c/ast/AstFloat.hpp>
#include <ace-c/ast/AstInteger.hpp>
#include <ace-c/ast/AstTrue.hpp>
#include <ace-c/ast/AstFalse.hpp>
#include <ace-c/ast/AstUndefined.hpp>
#include <ace-c/ast/AstNil.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/emit/Instruction.hpp>

#include <common/instructions.hpp>

#include <limits>
#include <cmath>

AstFloat::AstFloat(ace::afloat32 value, const SourceLocation &location)
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

void AstFloat::Recreate(std::ostringstream &ss)
{
    ss << m_value;
}

Pointer<AstStatement> AstFloat::Clone() const
{
    return CloneImpl();
}

int AstFloat::IsTrue() const
{
    // any non-zero value is considered true
    return m_value != 0.0f;
}

bool AstFloat::IsNumber() const
{
    return true;
}

ace::aint32 AstFloat::IntValue() const
{
    return (ace::aint32)m_value;
}

ace::afloat32 AstFloat::FloatValue() const
{
    return m_value;
}

SymbolTypePtr_t AstFloat::GetSymbolType() const
{
    return SymbolType::Builtin::FLOAT;
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

    auto right_float = right->FloatValue();
    if (right_float == 0.0) {
        // division by zero
        return nullptr;
    }
    return std::shared_ptr<AstFloat>(new AstFloat(FloatValue() / right_float, m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator%(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    auto right_float = right->FloatValue();
    if (right_float == 0.0) {
        // division by zero
        return nullptr;
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
        if (dynamic_cast<AstNil*>(right)) {
            // rhs is null, return false
            return std::shared_ptr<AstFalse>(new AstFalse(m_location));
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
        if (dynamic_cast<AstNil*>(right)) {
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

std::shared_ptr<AstConstant> AstFloat::operator<(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    if (FloatValue() < right->FloatValue()) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    } else {
        return std::shared_ptr<AstFalse>(new AstFalse(m_location));
    }
}

std::shared_ptr<AstConstant> AstFloat::operator>(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    if (FloatValue() > right->FloatValue()) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    } else {
        return std::shared_ptr<AstFalse>(new AstFalse(m_location));
    }
}

std::shared_ptr<AstConstant> AstFloat::operator<=(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    if (FloatValue() <= right->FloatValue()) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    } else {
        return std::shared_ptr<AstFalse>(new AstFalse(m_location));
    }
}

std::shared_ptr<AstConstant> AstFloat::operator>=(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    if (FloatValue() >= right->FloatValue()) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    } else {
        return std::shared_ptr<AstFalse>(new AstFalse(m_location));
    }
}

std::shared_ptr<AstConstant> AstFloat::Equals(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    if (FloatValue() == right->FloatValue()) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    } else {
        return std::shared_ptr<AstFalse>(new AstFalse(m_location));
    }
}

std::shared_ptr<AstConstant> AstFloat::operator-() const
{
    return std::shared_ptr<AstFloat>(new AstFloat(-FloatValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator~() const
{
    // invalid operator on floats
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator!() const
{
    if (FloatValue() == 0.0) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    } else {
        return std::shared_ptr<AstFalse>(new AstFalse(m_location));
    }
}
