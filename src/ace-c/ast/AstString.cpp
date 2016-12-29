#include <ace-c/ast/AstString.hpp>
#include <ace-c/ast/AstTrue.hpp>
#include <ace-c/ast/AstFalse.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/StaticObject.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>

AstString::AstString(const std::string &value, const SourceLocation &location)
    : AstConstant(location),
      m_value(value),
      m_static_id(0)
{
}

void AstString::Build(AstVisitor *visitor, Module *mod)
{
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    StaticObject so(m_value.c_str());

    int found_id = visitor->GetCompilationUnit()->GetInstructionStream().FindStaticObject(so);
    if (found_id == -1) {
        m_static_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();
        so.m_id = m_static_id;
        visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(so);
    } else {
        m_static_id = found_id;
    }

    // load static object into register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, m_static_id);

    if (!ace::compiler::Config::use_static_objects) {
        // fill with padding for LOAD_STRING instruction
        visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2 + std::strlen(so.m_value.str);
    }
}

void AstString::Recreate(std::ostringstream &ss)
{
    ss << "\"";
    for (char ch : m_value) {
        switch (ch) {
            case '\\': ss << "\\\\"; break;
            case '"':  ss << "\\\""; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default: ss << ch; break;
        }
    }
    ss << "\"";
}

int AstString::IsTrue() const
{
    // strings evaluate to true
    return 1;
}

ObjectType AstString::GetObjectType() const
{
    return ObjectType::type_builtin_string;
}

bool AstString::IsNumber() const
{
    return false;
}

a_int AstString::IntValue() const
{
    // not valid
    return 0;
}

a_float AstString::FloatValue() const
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