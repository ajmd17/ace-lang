#include <athens/ast/ast_integer.h>
#include <athens/ast/ast_float.h>

#include <limits>
#include <cmath>

AstInteger::AstInteger(a_int value, const SourceLocation &location)
    : AstConstant(location),
      m_value(value)
{
}

void AstInteger::Build(AstVisitor *visitor) const
{
}

int AstInteger::IsTrue() const
{
    // any non-zero value is considered true
    return m_value != 0;
}

a_int AstInteger::IntValue() const
{
    return m_value;
}

a_float AstInteger::FloatValue() const
{
    return static_cast<a_float>(m_value);
}

std::shared_ptr<AstConstant> AstInteger::operator+(
        const std::shared_ptr<AstConstant> &right) const
{
    // we have to determine weather or not to promote this to a float
    if (dynamic_cast<const AstFloat*>(right.get()) != nullptr) {
        return std::shared_ptr<AstFloat>(
            new AstFloat(FloatValue() + right->FloatValue(), m_location));
    } else {
        return std::shared_ptr<AstInteger>(
            new AstInteger(IntValue() + right->IntValue(), m_location));
    }
}

std::shared_ptr<AstConstant> AstInteger::operator-(
        const std::shared_ptr<AstConstant> &right) const
{
    // we have to determine weather or not to promote this to a float
    if (dynamic_cast<const AstFloat*>(right.get()) != nullptr) {
        return std::shared_ptr<AstFloat>(
            new AstFloat(FloatValue() - right->FloatValue(), m_location));
    } else {
        return std::shared_ptr<AstInteger>(
            new AstInteger(IntValue() - right->IntValue(), m_location));
    }
}

std::shared_ptr<AstConstant> AstInteger::operator*(
        const std::shared_ptr<AstConstant> &right) const
{
    // we have to determine weather or not to promote this to a float
    if (dynamic_cast<const AstFloat*>(right.get()) != nullptr) {
        return std::shared_ptr<AstFloat>(
            new AstFloat(FloatValue() * right->FloatValue(), m_location));
    } else {
        return std::shared_ptr<AstInteger>(
            new AstInteger(IntValue() * right->IntValue(), m_location));
    }
}

std::shared_ptr<AstConstant> AstInteger::operator/(
        const std::shared_ptr<AstConstant> &right) const
{
    // we have to determine weather or not to promote this to a float
    if (dynamic_cast<const AstFloat*>(right.get()) != nullptr) {
        a_float result;
        a_float right_float = right->FloatValue();
        if (right_float == 0.0) {
            // division by zero, return NaN
            result = std::numeric_limits<decltype(result)>::quiet_NaN();
        } else {
            result = FloatValue() / right_float;
        }
        return std::shared_ptr<AstFloat>(
            new AstFloat(result, m_location));
    } else {
        a_int right_int = right->IntValue();
        if (right_int == 0) {
            // division by zero, return NaN
            return std::shared_ptr<AstFloat>(
                new AstFloat(std::numeric_limits<a_float>::quiet_NaN(), m_location));
        } else {
            return std::shared_ptr<AstInteger>(
                new AstInteger(IntValue() / right_int, m_location));
        }
    }
}

std::shared_ptr<AstConstant> AstInteger::operator%(
        const std::shared_ptr<AstConstant> &right) const
{
    // we have to determine weather or not to promote this to a float
    if (dynamic_cast<const AstFloat*>(right.get()) != nullptr) {
        a_float result;
        a_float right_float = right->FloatValue();
        if (right_float == 0.0) {
            // division by zero, return NaN
            result = std::numeric_limits<decltype(result)>::quiet_NaN();
        } else {
            result = std::fmod(FloatValue(), right_float);
        }
        return std::shared_ptr<AstFloat>(
            new AstFloat(result, m_location));
    } else {
        a_int right_int = right->IntValue();
        if (right_int == 0) {
            // division by zero, return NaN
            return std::shared_ptr<AstFloat>(
                new AstFloat(std::numeric_limits<a_float>::quiet_NaN(), m_location));
        } else {
            return std::shared_ptr<AstInteger>(
                new AstInteger(IntValue() % right_int, m_location));
        }
    }
}

std::shared_ptr<AstConstant> AstInteger::operator^(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() ^ right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator&(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() & right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator|(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator<<(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() << right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator>>(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() >> right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator&&(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() && right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator||(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() || right->IntValue(), m_location));
}