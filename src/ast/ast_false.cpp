#include <athens/ast/ast_false.h>
#include <athens/ast/ast_integer.h>

AstFalse::AstFalse(const SourceLocation &location)
    : AstConstant(location)
{
}

void AstFalse::Build(AstVisitor *visitor) const
{
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

std::shared_ptr<AstConstant> AstFalse::operator+(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() + right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator-(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() - right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator*(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() * right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator/(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() - right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator%(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() % right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator^(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() ^ right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator&(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() & right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator|(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator<<(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() << right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator>>(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() >> right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator&&(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() && right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator||(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() || right->IntValue(), m_location));
}