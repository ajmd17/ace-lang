#include <athens/ast/ast_string.h>

AstString::AstString(const std::string &value, const SourceLocation &location)
    : AstConstant(location),
      m_value(value)
{
}

void AstString::Build(AstVisitor *visitor)
{
}

int AstString::IsTrue() const
{
    return -1;
}

bool AstString::IsNumber() const
{
    return false;
}

a_int AstString::IntValue() const
{
    return 0;
}

a_float AstString::FloatValue() const
{
    return 0.0f;
}

std::shared_ptr<AstConstant> AstString::operator+(
        AstConstant *right) const
{
    // TODO: string concatenation
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator-(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator*(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator/(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator%(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator^(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator&(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator|(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator<<(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator>>(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator&&(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator||(
        AstConstant *right) const
{
    return nullptr;
}