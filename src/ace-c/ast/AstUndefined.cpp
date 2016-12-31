#include <ace-c/ast/AstUndefined.hpp>

AstUndefined::AstUndefined(const SourceLocation &location)
    : AstConstant(location)
{
}

void AstUndefined::Build(AstVisitor *visitor, Module *mod)
{
}

void AstUndefined::Recreate(std::ostringstream &ss)
{
    ss << "??";
}

int AstUndefined::IsTrue() const
{
    return false;
}

bool AstUndefined::IsNumber() const
{
    return false;
}

a_int AstUndefined::IntValue() const
{
    return 0;
}

a_float AstUndefined::FloatValue() const
{
    return 0.0f;
}

SymbolTypePtr_t AstUndefined::GetSymbolType() const
{
    return SymbolType::Builtin::UNDEFINED;
}

std::shared_ptr<AstConstant> AstUndefined::operator+(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator-(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator*(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator/(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator%(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator^(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator&(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator|(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator<<(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator>>(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator&&(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator||(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator<(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator>(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator<=(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator>=(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::Equals(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator-() const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator~() const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstUndefined::operator!() const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}