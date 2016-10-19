#include <athens/ast/ast_float.hpp>
#include <athens/ast/ast_integer.hpp>
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

void AstFloat::Build(AstVisitor *visitor)
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

std::shared_ptr<AstConstant> AstFloat::operator+(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstFloat>(
        new AstFloat(FloatValue() + right->FloatValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator-(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstFloat>(
        new AstFloat(FloatValue() - right->FloatValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator*(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstFloat>(
        new AstFloat(FloatValue() * right->FloatValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator/(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    a_float result;
    a_float right_float = right->FloatValue();
    if (right_float == 0.0) {
        // division by zero, return NaN
        result = std::numeric_limits<decltype(result)>::quiet_NaN();
    } else {
        result = FloatValue() / right_float;
    }
    return std::shared_ptr<AstFloat>(new AstFloat(result, m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator%(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    a_float result;
    a_float right_float = right->FloatValue();
    if (right_float == 0.0) {
        // division by zero, return NaN
        result = std::numeric_limits<decltype(result)>::quiet_NaN();
    } else {
        result = std::fmod(FloatValue(), right_float);
    }
    return std::shared_ptr<AstFloat>(new AstFloat(result, m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator^(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    // demote to integer
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() ^ right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator&(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    // demote to integer
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() & right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator|(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    // demote to integer
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator<<(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    // demote to integer
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() << right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator>>(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    // demote to integer
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() >> right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator&&(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    // demote to integer
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() && right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator||(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    // demote to integer
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() || right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::Equals(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstFloat>(
        new AstFloat(FloatValue() == right->FloatValue(), m_location));
}
