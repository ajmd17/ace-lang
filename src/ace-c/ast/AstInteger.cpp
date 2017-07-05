#include <ace-c/ast/AstInteger.hpp>
#include <ace-c/ast/AstFloat.hpp>
#include <ace-c/ast/AstNil.hpp>
#include <ace-c/ast/AstTrue.hpp>
#include <ace-c/ast/AstFalse.hpp>
#include <ace-c/ast/AstUndefined.hpp>
#include <ace-c/AstVisitor.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/emit/BytecodeUtil.hpp>

#include <iostream>
#include <limits>
#include <cmath>

AstInteger::AstInteger(ace::aint32 value, const SourceLocation &location)
    : AstConstant(location),
      m_value(value)
{
}

std::unique_ptr<Buildable> AstInteger::Build(AstVisitor *visitor, Module *mod)
{
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    return BytecodeUtil::Make<ConstI32>(rp, m_value);
}

Pointer<AstStatement> AstInteger::Clone() const
{
    return CloneImpl();
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

ace::aint32 AstInteger::IntValue() const
{
    return m_value;
}

ace::afloat32 AstInteger::FloatValue() const
{
    return (ace::afloat32)m_value;
}

SymbolTypePtr_t AstInteger::GetSymbolType() const
{
    return BuiltinTypes::INT;
}

std::shared_ptr<AstConstant> AstInteger::operator+(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    // we have to determine weather or not to promote this to a float
    if (dynamic_cast<const AstFloat*>(right)) {
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
    if (dynamic_cast<const AstFloat*>(right)) {
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
    if (dynamic_cast<const AstFloat*>(right)) {
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
    if (dynamic_cast<const AstFloat*>(right)) {
        ace::afloat32 result;
        auto right_float = right->FloatValue();
        if (right_float == 0.0) {
            // division by zero, return Undefined
            return nullptr;
        } else {
            result = FloatValue() / right_float;
        }
        return std::shared_ptr<AstFloat>(
            new AstFloat(result, m_location));
    } else {
        auto right_int = right->IntValue();
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
    if (dynamic_cast<const AstFloat*>(right)) {
        ace::afloat32 result;
        auto right_float = right->FloatValue();
        if (right_float == 0.0) {
            // division by zero, return Undefined
            return nullptr;
        } else {
            result = std::fmod(FloatValue(), right_float);
        }
        return std::shared_ptr<AstFloat>(
            new AstFloat(result, m_location));
    } else {
        auto right_int = right->IntValue();
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
    if (!right->IsNumber() || right->GetSymbolType() != BuiltinTypes::INT) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() ^ right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator&(AstConstant *right) const
{
    // right must be integer
    if (!right->IsNumber() || right->GetSymbolType() != BuiltinTypes::INT) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() & right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator|(AstConstant *right) const
{
    // right must be integer
    if (!right->IsNumber() || right->GetSymbolType() != BuiltinTypes::INT) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator<<(AstConstant *right) const
{
    // right must be integer
    if (!right->IsNumber() || right->GetSymbolType() != BuiltinTypes::INT) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() << right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstInteger::operator>>(AstConstant *right) const
{
    // right must be integer
    if (!right->IsNumber() || right->GetSymbolType() != BuiltinTypes::INT) {
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

std::shared_ptr<AstConstant> AstInteger::operator||(AstConstant *right) const
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