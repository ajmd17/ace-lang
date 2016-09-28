#include <athens/ast/ast_null.h>
#include <athens/ast/ast_integer.h>

AstNull::AstNull(const SourceLocation &location)
    : AstConstant(location)
{
}

void AstNull::Build(AstVisitor *visitor) const
{
}

int AstNull::IsTrue() const
{
    return false;
}

a_int AstNull::IntValue() const
{
    return 0;
}

a_float AstNull::FloatValue() const
{
    return 0.0f;
}

std::shared_ptr<AstConstant> AstNull::operator+(
        const std::shared_ptr<AstConstant> &right) const
{
    // return null on any operation applied to the null keyword
    // TODO: make it add a compiler error
    return std::shared_ptr<AstNull>(new AstNull(m_location));
}

std::shared_ptr<AstConstant> AstNull::operator-(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstNull>(new AstNull(m_location));
}

std::shared_ptr<AstConstant> AstNull::operator*(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstNull>(new AstNull(m_location));
}

std::shared_ptr<AstConstant> AstNull::operator/(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstNull>(new AstNull(m_location));
}

std::shared_ptr<AstConstant> AstNull::operator%(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstNull>(new AstNull(m_location));
}

std::shared_ptr<AstConstant> AstNull::operator^(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstNull>(new AstNull(m_location));
}

std::shared_ptr<AstConstant> AstNull::operator&(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstNull>(new AstNull(m_location));
}

std::shared_ptr<AstConstant> AstNull::operator|(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstNull>(new AstNull(m_location));
}

std::shared_ptr<AstConstant> AstNull::operator&&(
        const std::shared_ptr<AstConstant> &right) const
{
    // logical operations still work, so that we can do
    // things like testing for null in an if statement.
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() && right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstNull::operator||(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() || right->IntValue(), m_location));
}