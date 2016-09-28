#include <athens/ast/ast_float.h>
#include <athens/ast/ast_integer.h>

#include <limits>
#include <cmath>

AstFloat::AstFloat(a_float value, const SourceLocation &location)
    : AstConstant(location),
      m_value(value)
{
}

void AstFloat::Build(AstVisitor *visitor) const
{
}

int AstFloat::IsTrue() const
{
    // any non-zero value is considered true
    return m_value != 0.0f;
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
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstFloat>(
        new AstFloat(FloatValue() + right->FloatValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator-(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstFloat>(
        new AstFloat(FloatValue() - right->FloatValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator*(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstFloat>(
        new AstFloat(FloatValue() * right->FloatValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator/(
        const std::shared_ptr<AstConstant> &right) const
{
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
        const std::shared_ptr<AstConstant> &right) const
{
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
        const std::shared_ptr<AstConstant> &right) const
{
    // demote to integer
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() ^ right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator&(
        const std::shared_ptr<AstConstant> &right) const
{
    // demote to integer
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() & right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator|(
        const std::shared_ptr<AstConstant> &right) const
{
    // demote to integer
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator&&(
        const std::shared_ptr<AstConstant> &right) const
{
    // demote to integer
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() && right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFloat::operator||(
        const std::shared_ptr<AstConstant> &right) const
{
    // demote to integer
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() || right->IntValue(), m_location));
}
