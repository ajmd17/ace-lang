#include <ace-c/ast/AstString.hpp>
#include <ace-c/ast/AstTrue.hpp>
#include <ace-c/ast/AstFalse.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Configuration.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/emit/BytecodeUtil.hpp>

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

Tribool AstString::IsTrue() const
{
    // strings evaluate to true
    return Tribool::True();
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

SymbolTypePtr_t AstString::GetExprType() const
{
    return BuiltinTypes::STRING;
}

std::shared_ptr<AstConstant> AstString::HandleOperator(Operators op_type, AstConstant *right) const
{
    switch (op_type) {
        case OP_logical_and:
            // literal strings evaluate to true.
            switch (right->IsTrue()) {
                case Tribool::TriboolValue::TRI_TRUE:
                    return std::shared_ptr<AstTrue>(new AstTrue(m_location));
                case Tribool::TriboolValue::TRI_FALSE:
                    return std::shared_ptr<AstFalse>(new AstFalse(m_location));
                case Tribool::TriboolValue::TRI_INDETERMINATE:
                    return nullptr;
            }

        case OP_logical_or:
            return std::shared_ptr<AstTrue>(new AstTrue(m_location));

        case OP_equals:
            if (AstString *right_string = dynamic_cast<AstString*>(right)) {
                if (m_value == right_string->GetValue()) {
                    return std::shared_ptr<AstTrue>(new AstTrue(m_location));
                } else {
                    return std::shared_ptr<AstFalse>(new AstFalse(m_location));
                }
            }

            return nullptr;

        default:
            return nullptr;
    }
}