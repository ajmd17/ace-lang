#include <athens/ast/ast_true.hpp>
#include <athens/ast/ast_integer.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/emit/instruction.hpp>

#include <common/instructions.hpp>

AstTrue::AstTrue(const SourceLocation &location)
    : AstConstant(location)
{
}

void AstTrue::Build(AstVisitor *visitor)
{
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // load value into register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t>(LOAD_TRUE, rp);
}

int AstTrue::IsTrue() const
{
    return true;
}

bool AstTrue::IsNumber() const
{
    return true;
}

a_int AstTrue::IntValue() const
{
    return 1;
}

a_float AstTrue::FloatValue() const
{
    return 1.0f;
}

std::shared_ptr<AstConstant> AstTrue::operator+(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() + right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator-(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() - right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator*(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() * right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator/(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() - right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator%(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() % right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator^(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() ^ right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator&(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() & right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator|(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator<<(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator>>(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator&&(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() && right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator||(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() || right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::Equals(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(right->FloatValue() != 0.0f, m_location));
}
