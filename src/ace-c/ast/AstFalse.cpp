#include <ace-c/ast/AstFalse.hpp>
#include <ace-c/ast/AstTrue.hpp>
#include <ace-c/ast/AstInteger.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/emit/Instruction.hpp>

#include <common/instructions.hpp>

AstFalse::AstFalse(const SourceLocation &location)
    : AstConstant(location)
{
}

void AstFalse::Build(AstVisitor *visitor, Module *mod)
{
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // load value into register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t>(LOAD_FALSE, rp);
}

void AstFalse::Recreate(std::ostringstream &ss)
{
    ss << Keyword::ToString(Keyword_false);
}

int AstFalse::IsTrue() const
{
    return false;
}

bool AstFalse::IsNumber() const
{
    return false;
}

a_int AstFalse::IntValue() const
{
    return 0;
}

a_float AstFalse::FloatValue() const
{
    return 0.0f;
}

SymbolTypePtr_t AstFalse::GetSymbolType() const
{
    return SymbolType::Builtin::BOOLEAN;
}

std::shared_ptr<AstConstant> AstFalse::operator+(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstFalse::operator-(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstFalse::operator*(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstFalse::operator/(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstFalse::operator%(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstFalse::operator^(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstFalse::operator&(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstFalse::operator|(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstFalse::operator<<(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstFalse::operator>>(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstFalse::operator&&(AstConstant *right) const
{
    return std::shared_ptr<AstFalse>(new AstFalse(m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator||(AstConstant *right) const
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

std::shared_ptr<AstConstant> AstFalse::operator<(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstFalse::operator>(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstFalse::operator<=(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstFalse::operator>=(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstFalse::Equals(AstConstant *right) const
{
    if (dynamic_cast<AstFalse*>(right) != nullptr) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    }
    return std::shared_ptr<AstFalse>(new AstFalse(m_location));
}

std::shared_ptr<AstConstant> AstFalse::operator-() const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstFalse::operator~() const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstFalse::operator!() const
{
    return std::shared_ptr<AstTrue>(new AstTrue(m_location));
}