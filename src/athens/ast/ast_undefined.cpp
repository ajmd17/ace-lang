#include <athens/ast/ast_undefined.hpp>

AstUndefined::AstUndefined(const SourceLocation &location)
    : AstConstant(location)
{
}

void AstUndefined::Build(AstVisitor *visitor, Module *mod)
{
}

int AstUndefined::IsTrue() const
{
    return false;
}

ObjectType AstUndefined::GetObjectType() const
{
    return ObjectType::type_builtin_undefined;
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

std::shared_ptr<AstConstant> AstUndefined::operator+(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator-(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator*(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator/(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator%(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator^(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator&(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator|(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator<<(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator>>(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator&&(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstVoid::operator||(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}

std::shared_ptr<AstConstant> AstVoid::Equals(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(m_location));
}
