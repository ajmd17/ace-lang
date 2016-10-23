#include <athens/ast/ast_variable.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/ast/ast_constant.hpp>
#include <athens/emit/instruction.hpp>

#include <common/instructions.hpp>

#include <iostream>

AstVariable::AstVariable(const std::string &name, const SourceLocation &location)
    : AstIdentifier(name, location)
{
}

void AstVariable::Visit(AstVisitor *visitor, Module *mod)
{
    AstIdentifier::Visit(visitor, mod);
}

void AstVariable::Build(AstVisitor *visitor, Module *mod)
{
    int stack_size = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
    int stack_location = m_identifier->GetStackLocation();
    int offset = stack_size - stack_location;

    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // load stack value at offset value into register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_LOCAL, rp, (uint16_t)offset);
}

void AstVariable::Optimize(AstVisitor *visitor, Module *mod)
{
}

int AstVariable::IsTrue() const
{
    if (m_identifier != nullptr) {
        // we can only check if this is true during
        // compile time if it is const literal
        if (m_identifier->GetFlags() & Flag_const) {
            auto value_sp = m_identifier->GetCurrentValue().lock();
            auto constant_sp = std::dynamic_pointer_cast<AstConstant>(value_sp);
            if (constant_sp != nullptr) {
                return constant_sp->IsTrue();
            }
        }
    }

    return -1;
}

bool AstVariable::MayHaveSideEffects() const
{
    // a simple variable reference does not cause side effects
    return false;
}
