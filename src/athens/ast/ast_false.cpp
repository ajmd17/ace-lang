#include <athens/ast/ast_false.h>
#include <athens/ast/ast_integer.h>
#include <athens/ast_visitor.h>
#include <athens/emit/instruction.h>

#include <common/instructions.h>

AstFalse::AstFalse(const SourceLocation &location)
    : AstConstant(location)
{
}

void AstFalse::Build(AstVisitor *visitor)
{
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // load value into register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t>(LOAD_FALSE, rp);
}

int AstFalse::IsTrue() const
{
    return false;
}

bool AstFalse::IsNumber() const
{
    return true;
}

a_int AstFalse::IntValue() const
{
    return 0;
}

a_float AstFalse::FloatValue() const
{
    return 0.0f;
}

std::shared_ptr<AstConstant> AstFalse::operator+(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() + right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator-(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() - right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator*(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() * right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator/(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() - right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator%(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() % right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator^(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() ^ right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator&(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() & right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator|(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator<<(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() << right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator>>(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() >> right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator&&(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() && right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator||(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() || right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::Equals(AstConstant *right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(right->FloatValue() != 0.0f, m_location));
}
