#include <athens/ast/ast_string.h>
#include <athens/ast_visitor.h>
#include <athens/emit/instruction.h>
#include <athens/emit/static_object.h>

#include <common/instructions.h>

AstString::AstString(const std::string &value, const SourceLocation &location)
    : AstConstant(location),
      m_value(value),
      m_static_id(0)
{
}

void AstString::Build(AstVisitor *visitor)
{
    m_static_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();
    
    StaticString ss;
    ss.m_id = m_static_id;
    ss.m_value = m_value;
    visitor->GetCompilationUnit()->GetInstructionStream().AddStaticString(ss);

    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // load static object into register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint32_t>(
            STORE_STATIC_STRING, rp, m_static_id);
}

int AstString::IsTrue() const
{
    return -1;
}

bool AstString::IsNumber() const
{
    return false;
}

a_int AstString::IntValue() const
{
    return 0;
}

a_float AstString::FloatValue() const
{
    return 0.0f;
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
    return nullptr;
}

std::shared_ptr<AstConstant> AstString::operator||(
        AstConstant *right) const
{
    return nullptr;
}