#include <ace-c/ast/AstInteger.hpp>
#include <ace-c/ast/AstFloat.hpp>
#include <ace-c/ast/AstNil.hpp>
#include <ace-c/ast/AstTrue.hpp>
#include <ace-c/ast/AstFalse.hpp>
#include <ace-c/ast/AstUndefined.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/emit/Instruction.hpp>

#include <common/instructions.hpp>

#include <iostream>
#include <limits>
#include <cmath>

AstInteger::AstInteger(a_int value, const SourceLocation &location)
    : AstConstant(location),
      m_value(value)
{
}

void AstInteger::Build(AstVisitor *visitor, Module *mod)
{
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // load integer value into register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, int32_t>(LOAD_I32, rp, m_value);
}

void AstInteger::Recreate(std::ostringstream &ss)
{
    ss << m_value;
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

SymbolTypePtr_t AstInteger::GetSymbolType() const
{
    return SymbolType::Builtin::INT;
}

std::shared_ptr<AstConstant> AstInteger::operator+(AstConstant *right) const
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

std::shared_ptr<AstConstant> AstInteger::operator-(AstConstant *right) const
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

std::shared_ptr<AstConstant> AstInteger::operator*(AstConstant *right) const
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

std::shared_ptr<AstConstant> AstInteger::operator/(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    // we have to determine weather or not to promote this to a float
    if (dynamic_cast<const AstFloat*>(right) != nullptr) {
        a_float result;
        a_float right_float = right->FloatValue();
        if (right_float == 0.0) {
            // division by zero, return Undefined
            return nullptr;
        } else {
            result = FloatValue() / right_float;
        }
        return std::shared_ptr<AstFloat>(
            new AstFloat(result, m_location));
    } else {
        a_int right_int = right->IntValue();
        if (right_int == 0) {
            // division by zero, return Undefined
            return nullptr;
        } else {
            return std::shared_ptr<AstInteger>(
                new AstInteger(IntValue() / right_int, m_location));
        }
    }
}

std::shared_ptr<AstConstant> AstInteger::operator%(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    // we have to determine weather or not to promote this to a float
    if (dynamic_cast<const AstFloat*>(right) != nullptr) {
        a_float result;
        a_float right_float = right->FloatValue();
        if (right_float == 0.0) {
            // division by zero, return Undefined
            return nullptr;
        } else {
            result = std::fmod(FloatValue(), right_float);
        }
        return std::shared_ptr<AstFloat>(
            new AstFloat(result, m_location));
    } else {
        a_int right_int = right->IntValue();
        if (right_int == 0) {
            // division by zero, return Undefined
            return nullptr;
        } else {
            return std::shared_ptr<AstInteger>(
                new AstInteger(IntValue() % right_int, m_location));
        }
    }
}

std::shared_ptr<AstConstant> AstInteger::operator^(AstConstant *right) const
{
    // right must be integer
    if (!right->IsNumber() || right->GetSymbolType() != SymbolType::Builtin::INT) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() ^ right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator&(AstConstant *right) const
{
    // right must be integer
    if (!right->IsNumber() || right->GetSymbolType() != SymbolType::Builtin::INT) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() & right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator|(AstConstant *right) const
{
    // right must be integer
    if (!right->IsNumber() || right->GetSymbolType() != SymbolType::Builtin::INT) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator<<(AstConstant *right) const
{
    // right must be integer
    if (!right->IsNumber() || right->GetSymbolType() != SymbolType::Builtin::INT) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() << right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator>>(AstConstant *right) const
{
    // right must be integer
    if (!right->IsNumber() || right->GetSymbolType() != SymbolType::Builtin::INT) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() >> right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator&&(AstConstant *right) const
{
    int this_true = IsTrue();
    int right_true = right->IsTrue();

    if (!right->IsNumber()) {
        // this operator is valid to compare against null
        if (dynamic_cast<AstNil*>(right)) {
            // rhs is nil, return false
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

std::shared_ptr<AstConstant> AstInteger::operator||(AstConstant *right) const
{
    int this_true = IsTrue();
    int right_true = right->IsTrue();

    if (!right->IsNumber()) {
        // this operator is valid to compare against nil
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

std::shared_ptr<AstConstant> AstInteger::operator<(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    if (IntValue() < right->IntValue()) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    } else {
        return std::shared_ptr<AstFalse>(new AstFalse(m_location));
    }
}

std::shared_ptr<AstConstant> AstInteger::operator>(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    if (IntValue() > right->IntValue()) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    } else {
        return std::shared_ptr<AstFalse>(new AstFalse(m_location));
    }
}

std::shared_ptr<AstConstant> AstInteger::operator<=(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    if (IntValue() <= right->IntValue()) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    } else {
        return std::shared_ptr<AstFalse>(new AstFalse(m_location));
    }
}

std::shared_ptr<AstConstant> AstInteger::operator>=(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    if (IntValue() >= right->IntValue()) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    } else {
        return std::shared_ptr<AstFalse>(new AstFalse(m_location));
    }
}

std::shared_ptr<AstConstant> AstInteger::Equals(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    if (IntValue() == right->IntValue()) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    } else {
        return std::shared_ptr<AstFalse>(new AstFalse(m_location));
    }
}

std::shared_ptr<AstConstant> AstInteger::operator-() const
{
    return std::shared_ptr<AstInteger>(new AstInteger(-IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator~() const
{
    return std::shared_ptr<AstInteger>(new AstInteger(~IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator!() const
{
    if (IntValue() == 0) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    } else {
        return std::shared_ptr<AstFalse>(new AstFalse(m_location));
    }
}