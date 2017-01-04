#include <ace-c/ast/AstNil.hpp>
#include <ace-c/ast/AstInteger.hpp>
#include <ace-c/ast/AstFalse.hpp>
#include <ace-c/ast/AstTrue.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/emit/Instruction.hpp>

#include <common/instructions.hpp>

AstNil::AstNil(const SourceLocation &location)
    : AstConstant(location)
{
}

void AstNil::Build(AstVisitor *visitor, Module *mod)
{
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // load integer value into register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t>(LOAD_NULL, rp);
}

void AstNil::Recreate(std::ostringstream &ss)
{
    ss << Keyword::ToString(Keyword_nil);
}

Pointer<AstStatement> AstNil::Clone() const
{
    return CloneImpl();
}

int AstNil::IsTrue() const
{
    return false;
}

bool AstNil::IsNumber() const
{
    return false;
}

ace::aint32 AstNil::IntValue() const
{
    return 0;
}

ace::afloat32 AstNil::FloatValue() const
{
    return 0.0f;
}

SymbolTypePtr_t AstNil::GetSymbolType() const
{
    return SymbolType::Builtin::ANY;
}

std::shared_ptr<AstConstant> AstNil::operator+(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNil::operator-(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNil::operator*(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNil::operator/(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNil::operator%(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNil::operator^(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNil::operator&(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNil::operator|(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNil::operator<<(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNil::operator>>(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNil::operator&&(AstConstant *right) const
{
    // logical operations still work, so that we can do
    // things like testing for null in an if statement.
    return std::shared_ptr<AstFalse>(new AstFalse(m_location));
}

std::shared_ptr<AstConstant> AstNil::operator||(AstConstant *right) const
{
    if (!right->IsNumber()) {
        // this operator is valid to compare against null
        if (AstNil *ast_null = dynamic_cast<AstNil*>(right)) {
            return std::shared_ptr<AstFalse>(new AstFalse(m_location));
        }
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstNil::operator<(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNil::operator>(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNil::operator<=(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNil::operator>=(AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNil::Equals(AstConstant *right) const
{
    if (dynamic_cast<AstNil*>(right)) {
        // only another null value should be equal
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    }
    // other values never equal to null
    return std::shared_ptr<AstFalse>(new AstFalse(m_location));
}

std::shared_ptr<AstConstant> AstNil::operator-() const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNil::operator~() const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNil::operator!() const
{
    return std::shared_ptr<AstTrue>(new AstTrue(m_location));
}