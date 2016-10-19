#include <athens/ast/ast_string.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/emit/instruction.hpp>
#include <athens/emit/static_object.hpp>

#include <common/instructions.hpp>

AstString::AstString(const std::string &value, const SourceLocation &location)
    : AstConstant(location),
      m_value(value),
      m_static_id(0)
{
}

void AstString::Build(AstVisitor *visitor)
{
    StaticObject so(m_value.c_str());

    int found_id = visitor->GetCompilationUnit()->GetInstructionStream().FindStaticObject(so);
    if (found_id == -1) {
        m_static_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();
        so.m_id = m_static_id;
        visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(so);
    } else {
        m_static_id = found_id;
    }

    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // load static object into register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint16_t>(
            LOAD_STATIC, rp, m_static_id);
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

std::shared_ptr<AstConstant> AstString::Equals(AstConstant *right) const
{
    // TODO
    return nullptr;
}
