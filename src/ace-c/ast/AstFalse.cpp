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

Tribool AstFalse::IsTrue() const
{
    return Tribool::False();
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

SymbolTypePtr_t AstFalse::GetExprType() const
{
    return BuiltinTypes::BOOLEAN;
}

std::shared_ptr<AstConstant> AstFalse::HandleOperator(Operators op_type, const AstConstant *right) const
{
    switch (op_type) {
        case OP_logical_and:
            return std::shared_ptr<AstFalse>(new AstFalse(m_location));

        case OP_logical_or:
            switch (right->IsTrue()) {
                case Tribool::TriboolValue::TRI_TRUE:
                    return std::shared_ptr<AstTrue>(new AstTrue(m_location));
                case Tribool::TriboolValue::TRI_FALSE:
                    return std::shared_ptr<AstFalse>(new AstFalse(m_location));
                case Tribool::TriboolValue::TRI_INDETERMINATE:
                    return nullptr;
            }

        case OP_equals:
            if (dynamic_cast<const AstFalse*>(right) != nullptr) {
                return std::shared_ptr<AstTrue>(new AstTrue(m_location));
            }
            return std::shared_ptr<AstFalse>(new AstFalse(m_location));

        case OP_logical_not:
            return std::shared_ptr<AstTrue>(new AstTrue(m_location));

        default:
            return nullptr;
    }
}