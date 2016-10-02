#include <athens/ast/ast_void.h>

AstVoid::AstVoid(const SourceLocation &location)
    : AstConstant(location)
{
}

void AstVoid::Build(AstVisitor *visitor) const
{
}

int AstVoid::IsTrue() const
{
    return false;
}

a_int AstVoid::IntValue() const
{
    return 0;
}

a_float AstVoid::FloatValue() const
{
    return 0.0f;
}

std::shared_ptr<AstConstant> AstVoid::operator+(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstVoid>(new AstVoid(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator-(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstVoid>(new AstVoid(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator*(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstVoid>(new AstVoid(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator/(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstVoid>(new AstVoid(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator%(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstVoid>(new AstVoid(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator^(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstVoid>(new AstVoid(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator&(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstVoid>(new AstVoid(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator|(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstVoid>(new AstVoid(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator<<(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstVoid>(new AstVoid(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator>>(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstVoid>(new AstVoid(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator&&(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstVoid>(new AstVoid(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator||(
        const std::shared_ptr<AstConstant> &right) const
{
    return std::shared_ptr<AstVoid>(new AstVoid(m_location));
}