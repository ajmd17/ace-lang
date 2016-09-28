#include <athens/ast/ast_true.h>
#include <athens/ast/ast_integer.h>

AstTrue::AstTrue(const SourceLocation &location)
    : AstConstant(location)
{
}

void AstTrue::Build(AstVisitor *visitor) const
{
}

int AstTrue::IsTrue() const
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
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() + right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator-(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() - right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator*(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() * right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator/(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() - right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator%(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() % right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator^(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() ^ right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator&(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() & right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator|(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator<<(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator>>(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator&&(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() && right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator||(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() || right->IntValue(), m_location));
}