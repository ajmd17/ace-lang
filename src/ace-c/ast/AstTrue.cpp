#include <ace-c/ast/AstTrue.hpp>
#include <ace-c/ast/AstFalse.hpp>
#include <ace-c/ast/AstInteger.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/emit/Instruction.hpp>

#include <common/instructions.hpp>

AstTrue::AstTrue(const SourceLocation &location)
    : AstConstant(location)
{
}

std::unique_ptr<Buildable> AstTrue::Build(AstVisitor *visitor, Module *mod)
{
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    // load value into register
    auto instr_load_true = BytecodeUtil::Make<RawOperation<>>();
    instr_load_true->opcode = LOAD_TRUE;
    instr_load_true->Accept<uint8_t>(rp);

    return std::move(instr_load_true);
}

void AstTrue::Recreate(std::ostringstream &ss)
{
    ss << Keyword::ToString(Keyword_true);
}

Pointer<AstStatement> AstTrue::Clone() const
{
    return CloneImpl();
}

int AstTrue::IsTrue() const
{
    return true;
}

bool AstTrue::IsNumber() const
{
    return false;
}

ace::aint32 AstTrue::IntValue() const
{
    return 1;
}

ace::afloat32 AstTrue::FloatValue() const
{
    return 1.0f;
}

SymbolTypePtr_t AstTrue::GetSymbolType() const
{
    return SymbolType::Builtin::BOOLEAN;
}

std::shared_ptr<AstConstant> AstTrue::operator+(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator-(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator*(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator/(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator%(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator^(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator&(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator|(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator<<(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator>>(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator&&(AstConstant *right) const
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

std::shared_ptr<AstConstant> AstTrue::operator||(AstConstant *right) const
{
    return std::shared_ptr<AstTrue>(new AstTrue(m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator<(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator>(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator<=(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstTrue::operator>=(AstConstant *right) const
{
    return nullptr;
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
