#include <ace-c/ast/ast_null.hpp>
#include <ace-c/ast/ast_integer.hpp>
#include <ace-c/ast/ast_void.hpp>
#include <ace-c/ast/ast_false.hpp>
#include <ace-c/ast/ast_true.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/emit/instruction.hpp>

#include <common/instructions.hpp>

AstNull::AstNull(const SourceLocation &location)
    : AstConstant(location)
{
}

void AstNull::Build(AstVisitor *visitor, Module *mod)
{
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // load integer value into register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t>(LOAD_NULL, rp);
}

int AstNull::IsTrue() const
{
    return false;
}

ObjectType AstNull::GetObjectType() const
{
    return ObjectType::type_builtin_any;
}

bool AstNull::IsNumber() const
{
    /** Set it to be a number so we can perform logical operations */
    return false;
}

a_int AstNull::IntValue() const
{
    return 0;
}

a_float AstNull::FloatValue() const
{
    return 0.0f;
}

std::shared_ptr<AstConstant> AstNull::operator+(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNull::operator-(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNull::operator*(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNull::operator/(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNull::operator%(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNull::operator^(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNull::operator&(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNull::operator|(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNull::operator<<(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNull::operator>>(
        AstConstant *right) const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNull::operator&&(
        AstConstant *right) const
{
    // logical operations still work, so that we can do
    // things like testing for null in an if statement.
    return std::shared_ptr<AstFalse>(new AstFalse(m_location));
}

std::shared_ptr<AstConstant> AstNull::operator||(
        AstConstant *right) const
{
    if (!right->IsNumber()) {
        // this operator is valid to compare against null
        AstNull *ast_null = dynamic_cast<AstNull*>(right);
        if (ast_null != nullptr) {
            return std::shared_ptr<AstFalse>(new AstFalse(m_location));
        }
        return nullptr;
    }

    return std::shared_ptr<AstInteger>(
        new AstInteger(right->IntValue(), m_location));
}

std::shared_ptr<AstConstant> AstNull::Equals(AstConstant *right) const
{
    if (dynamic_cast<AstNull*>(right) != nullptr) {
        // only another null value should be equal
        return std::shared_ptr<AstTrue>(new AstTrue(m_location));
    }
    // other values never equal to null
    return std::shared_ptr<AstFalse>(new AstFalse(m_location));
}

std::shared_ptr<AstConstant> AstNull::operator-() const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNull::operator~() const
{
    return nullptr;
}

std::shared_ptr<AstConstant> AstNull::operator!() const
{
    return std::shared_ptr<AstTrue>(new AstTrue(m_location));
}