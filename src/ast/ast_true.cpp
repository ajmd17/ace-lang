#include <athens/ast/ast_true.h>
#include <athens/ast/ast_integer.h>
#include <athens/ast_visitor.h>
#include <athens/emit/instruction.h>

#include <common/instructions.h>

AstTrue::AstTrue(const SourceLocation &location)
    : AstConstant(location)
{
}

void AstTrue::Build(AstVisitor *visitor)
{
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // load integer (1) value into register
    visitor->GetCompilationUnit()->GetInstructionStream() << 
        Instruction<uint8_t, uint8_t, int32_t>(LOAD_I32, rp, 1);
}

int AstTrue::IsTrue() const
{
    return true;
}

bool AstTrue::IsNumber() const
{
    return true;
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
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() + right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator-(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() - right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator*(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() * right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator/(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() - right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator%(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() % right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator^(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() ^ right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator&(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() & right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator|(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator<<(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator>>(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() | right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator&&(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() && right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstTrue::operator||(
        const std::shared_ptr<AstConstant> &right) const
{
    if (!right->IsNumber()) {
        return nullptr;
    }
    
    return std::shared_ptr<AstInteger>(
        new AstInteger(IntValue() || right->IntValue(), m_location));
}