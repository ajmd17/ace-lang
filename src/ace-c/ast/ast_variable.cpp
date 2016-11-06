#include <ace-c/ast/ast_variable.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/ast/ast_constant.hpp>
#include <ace-c/emit/instruction.hpp>

#include <common/instructions.hpp>

#include <cassert>
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
    assert(m_identifier != nullptr);

    int stack_size = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
    int stack_location = m_identifier->GetStackLocation();
    int offset = stack_size - stack_location;

    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    if (m_in_function && !(m_identifier->GetFlags() & FLAG_DECLARED_IN_FUNCTION)) {
        // we must load globally, rather than from offset.
        // we are within a function right now, but loading a variable not declared in a function
        if (m_access_mode == ACCESS_MODE_LOAD) {
            // load stack value at index into register
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t, uint16_t>(LOAD_INDEX, rp, (uint16_t)stack_location);
        } else if (m_access_mode == ACCESS_MODE_STORE) {
            // store the value at the index into this local variable
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint16_t, uint8_t>(MOV_INDEX, (uint16_t)stack_location, rp - 1);
        }
    } else {
        if (m_access_mode == ACCESS_MODE_LOAD) {
            // load stack value at offset value into register
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t, uint16_t>(LOAD_OFFSET, rp, (uint16_t)offset);
        } else if (m_access_mode == ACCESS_MODE_STORE) {
            // store the value at (rp - 1) into this local variable
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint16_t, uint8_t>(MOV_OFFSET, (uint16_t)offset, rp - 1);
        }
    }
}

void AstVariable::Optimize(AstVisitor *visitor, Module *mod)
{
}

int AstVariable::IsTrue() const
{
    if (m_identifier != nullptr) {
        // we can only check if this is true during
        // compile time if it is const literal
        if (m_identifier->GetFlags() & FLAG_CONST) {
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
