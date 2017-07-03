#include <ace-c/ast/AstString.hpp>
#include <ace-c/ast/AstTrue.hpp>
#include <ace-c/ast/AstFalse.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Configuration.hpp>

#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/instructions.hpp>

AstString::AstString(const std::string &value, const SourceLocation &location)
    : AstConstant(location),
      m_value(value),
      m_static_id(0)
{
}

std::unique_ptr<Buildable> AstString::Build(AstVisitor *visitor, Module *mod)
{
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    auto instr_string = BytecodeUtil::Make<BuildableString>();
    instr_string->reg = rp;
    instr_string->value = m_value;

    return std::move(instr_string);
}

Pointer<AstStatement> AstString::Clone() const
{
    return CloneImpl();
}

int AstString::IsTrue() const
{
    // strings evaluate to true
    return 1;
}

bool AstString::IsNumber() const
{
    return false;
}

ace::aint32 AstString::IntValue() const
{
    // not valid
    return 0;
}

ace::afloat32 AstString::FloatValue() const
{
    // not valid
    return 0.0f;
}

SymbolTypePtr_t AstString::GetSymbolType() const
{
    return SymbolType::Builtin::STRING;
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
    // string literals evaluate to true
    bool right_true = right->IsTrue();
    if (right_true == 1) {
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    } else if (right_true == 0) {
        return std::shared_ptr<AstFalse>(new AstFalse(m_location));
    } else {
        return nullptr;
    }
}

std::shared_ptr<AstConstant> AstString::operator||(
        AstConstant *right) const
{
    return std::shared_ptr<AstTrue>(new AstTrue(m_location));
}


std::shared_ptr<AstConstant> AstString::operator<(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator>(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator<=(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator>=(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::Equals(AstConstant *right) const
{
    // TODO
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator-() const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator~() const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator!() const
{
    return std::shared_ptr<AstFalse>(new AstFalse(m_location));
}