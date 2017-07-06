#include <ace-c/ast/AstTrue.hpp>
#include <ace-c/ast/AstFalse.hpp>
#include <ace-c/ast/AstInteger.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Keywords.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/emit/BytecodeUtil.hpp>

AstTrue::AstTrue(const SourceLocation &location)
    : AstConstant(location)
{
}

std::unique_ptr<Buildable> AstTrue::Build(AstVisitor *visitor, Module *mod)
{
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    return BytecodeUtil::Make<ConstBool>(rp, true);
}

Pointer<AstStatement> AstTrue::Clone() const
{
    return CloneImpl();
}

Tribool AstTrue::IsTrue() const
{
    return Tribool::True();
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
    return BuiltinTypes::BOOLEAN;
}

std::shared_ptr<AstConstant> AstTrue::HandleOperator(Operators op_type, AstConstant *right) const
{
    switch (op_type) {
        case OP_logical_and:
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
            if (dynamic_cast<AstTrue*>(right) != nullptr) {
                return std::shared_ptr<AstTrue>(new AstTrue(m_location));
            }
            return std::shared_ptr<AstFalse>(new AstFalse(m_location));

        case OP_logical_not:
            return std::shared_ptr<AstFalse>(new AstFalse(m_location));

        default:
            return nullptr;
    }
}