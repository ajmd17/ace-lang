#include <ace-c/ast/AstFalse.hpp>
#include <ace-c/ast/AstTrue.hpp>
#include <ace-c/ast/AstInteger.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Keywords.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/emit/BytecodeUtil.hpp>

AstFalse::AstFalse(const SourceLocation &location)
    : AstConstant(location)
{
}

std::unique_ptr<Buildable> AstFalse::Build(AstVisitor *visitor, Module *mod)
{
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    return BytecodeUtil::Make<ConstBool>(rp, false);
}

Pointer<AstStatement> AstFalse::Clone() const
{
    return CloneImpl();
}

int AstFalse::IsTrue() const
{
    return false;
}

bool AstFalse::IsNumber() const
{
    return false;
}

ace::aint32 AstFalse::IntValue() const
{
    return 0;
}

ace::afloat32 AstFalse::FloatValue() const
{
    return 0.0f;
}

SymbolTypePtr_t AstFalse::GetSymbolType() const
{
    return BuiltinTypes::BOOLEAN;
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
    int right_true = right->IsTrue();
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