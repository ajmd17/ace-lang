#include <ace-c/ast/ast_void.hpp>
#include <ace-c/ast/ast_undefined.hpp>

AstVoid::AstVoid(const SourceLocation &location)
    : AstConstant(location)
{
}

void AstVoid::Build(AstVisitor *visitor, Module *mod)
{
}

int AstVoid::IsTrue() const
{
    return false;
}

ObjectType AstVoid::GetObjectType() const
{
    return ObjectType::type_builtin_void;
}

bool AstVoid::IsNumber() const
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

std::shared_ptr<AstConstant> AstVoid::operator+(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator-(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator*(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator/(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator%(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator^(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator&(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator|(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator<<(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator>>(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator&&(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator||(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator<(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator>(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator<=(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator>=(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::Equals(AstConstant *right) const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator-() const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator~() const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}

std::shared_ptr<AstConstant> AstVoid::operator!() const
{
    return std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof));
}