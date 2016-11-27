#include <ace-c/ast/ast_true.hpp>
#include <ace-c/ast/ast_false.hpp>
#include <ace-c/ast/ast_integer.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/emit/instruction.hpp>

#include <common/instructions.hpp>

AstTrue::AstTrue(const SourceLocation &location)
    : AstConstant(location)
{
}

void AstTrue::Build(AstVisitor *visitor, Module *mod)
{
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // load value into register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t>(LOAD_TRUE, rp);
}

int AstTrue::IsTrue() const
{
    return true;
}

ObjectType AstTrue::GetObjectType() const
{
    return ObjectType::type_builtin_bool;
}

bool AstTrue::IsNumber() const
{
    return false;
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
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator-(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator*(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator/(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator%(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator^(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator&(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator|(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator<<(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator>>(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator&&(
        AstConstant *right) const
{
    bool right_true = right->IsTrue();
    if (right_true == 1) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    } else if (right_true == 0) {
        return std::shared_ptr<AstFalse>(new AstFalse(m_location));
    } else {
        return nullptr;
    }
}

std::shared_ptr<AstConstant> AstTrue::operator||(
        AstConstant *right) const
{
    return std::shared_ptr<AstTrue>(new AstTrue(m_location));
}

std::shared_ptr<AstConstant> AstTrue::Equals(AstConstant *right) const
{
    if (dynamic_cast<AstTrue*>(right) != nullptr) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    }
    return std::shared_ptr<AstFalse>(new AstFalse(m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator-() const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator~() const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator!() const
{
    return std::shared_ptr<AstFalse>(new AstFalse(m_location));
}