#include <ace-c/ast/AstUndefined.hpp>

AstUndefined::AstUndefined(const SourceLocation &location)
    : AstConstant(location)
{
}

std::unique_ptr<Buildable> AstUndefined::Build(AstVisitor *visitor, Module *mod)
{
    return nullptr;
}

void AstUndefined::Recreate(std::ostringstream &ss)
{
    ss << "??";
}

Pointer<AstStatement> AstUndefined::Clone() const
{
    return CloneImpl();
}

int AstUndefined::IsTrue() const
{
    return false;
}

bool AstUndefined::IsNumber() const
{
    return false;
}

ace::aint32 AstUndefined::IntValue() const
{
    return 0;
}

ace::afloat32 AstUndefined::FloatValue() const
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