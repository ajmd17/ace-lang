#include <athens/ast/ast_integer.hpp>
#include <athens/ast/ast_float.hpp>
#include <athens/ast/ast_null.hpp>
#include <athens/ast/ast_false.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/emit/instruction.hpp>

#include <common/instructions.hpp>

#include <iostream>
#include <limits>
#include <cmath>

AstInteger::AstInteger(a_int value, const SourceLocation &location)
    : AstConstant(location),
      m_value(value)
{
}

void AstInteger::Build(AstVisitor *visitor)
{
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // load integer value into register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, int32_t>(LOAD_I32, rp, m_value);
}

int AstInteger::IsTrue() const
{
    // any non-zero value is considered true
    return m_value != 0;
}

bool AstInteger::IsNumber() const
{
    return true;
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
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    // we have to determine weather or not to promote this to a float
    if (dynamic_cast<const AstFloat*>(right) != nullptr) {
        return std::shared_ptr<AstFloat>(
            new AstFloat(FloatValue() + right->FloatValue(), m_location));
    } else {
        return std::shared_ptr<AstInteger>(
            new AstInteger(IntValue() + right->IntValue(), m_location));
    }
}

std::shared_ptr<AstConstant> AstInteger::operator-(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    // we have to determine weather or not to promote this to a float
    if (dynamic_cast<const AstFloat*>(right) != nullptr) {
        return std::shared_ptr<AstFloat>(
            new AstFloat(FloatValue() - right->FloatValue(), m_location));
    } else {
        return std::shared_ptr<AstInteger>(
            new AstInteger(IntValue() - right->IntValue(), m_location));
    }
}

std::shared_ptr<AstConstant> AstInteger::operator*(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    // we have to determine weather or not to promote this to a float
    if (dynamic_cast<const AstFloat*>(right) != nullptr) {
        return std::shared_ptr<AstFloat>(
            new AstFloat(FloatValue() * right->FloatValue(), m_location));
    } else {
        return std::shared_ptr<AstInteger>(
            new AstInteger(IntValue() * right->IntValue(), m_location));
    }
}

std::shared_ptr<AstConstant> AstInteger::operator/(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    // we have to determine weather or not to promote this to a float
    if (dynamic_cast<const AstFloat*>(right) != nullptr) {
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
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    // we have to determine weather or not to promote this to a float
    if (dynamic_cast<const AstFloat*>(right) != nullptr) {
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
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() ^ right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator&(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() & right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator|(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator<<(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() << right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator>>(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() >> right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator&&(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        // this operator is valid to compare against null
        AstNull *ast_null = dynamic_cast<AstNull*>(right);
        if (ast_null != nullptr) {
            return std::shared_ptr<AstFalse>(
                new AstFalse(m_location));
        }
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() && right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator||(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        // this operator is valid to compare against null
        AstNull *ast_null = dynamic_cast<AstNull*>(right);
        if (ast_null != nullptr) {
            return std::shared_ptr<AstInteger>(
                new AstInteger(IntValue() || 0, m_location));
        }
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() || right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::Equals(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() == right->IntValue(), m_location));
}
