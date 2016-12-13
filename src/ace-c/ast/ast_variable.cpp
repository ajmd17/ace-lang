#include <ace-c/ast/ast_variable.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/ast/ast_constant.hpp>
#include <ace-c/emit/instruction.hpp>

#include <common/instructions.hpp>

#include <common/my_assert.hpp>
#include <iostream>

AstVariable::AstVariable(const std::string &name, const SourceLocation &location)
    : AstIdentifier(name, location)
{
}

void AstVariable::Visit(AstVisitor *visitor, Module *mod)
{
    AstIdentifier::Visit(visitor, mod);

    if (m_identifier != nullptr) {
        // NOTE: if whe're are in a function, and the variable we are loading is declared in a separate function,
        // we will show an error message saying that the variable must be passed as a parameter to be captured.
        // the reason for this is that any variables owned by the parent function will be immediately popped from the stack
        // when the parent function returns. That will mean the variables used here will reference garbage.
        // In the near feature, it'd be possible to automatically make a copy of those variables referenced and store them
        // on the stack of /this/ function.

        if (m_in_function && (m_identifier->GetFlags() & FLAG_DECLARED_IN_FUNCTION)) {
            // lookup the variable by depth to make sure it was declared in the current function
            auto identifer_this_scope = mod->LookUpIdentifierDepth(m_name, m_depth);

            // we do this to make sure it was declared in this scope.
            if (identifer_this_scope == nullptr) {
                // add error that the variable must be passed as a parameter
                visitor->GetCompilationUnit()->GetErrorList().AddError(
                    CompilerError(Level_fatal, Msg_closure_capture_must_be_parameter,
                        m_location, m_name));
            }
        }
    }
}

void AstVariable::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_identifier != nullptr);

    int stack_size = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
    int stack_location = m_identifier->GetStackLocation();
    int offset = stack_size - stack_location;

    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    if (!(m_identifier->GetFlags() & FLAG_DECLARED_IN_FUNCTION)) {
        // load globally, rather than from offset.
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
            auto value_sp = m_identifier->GetCurrentValue();
            AstConstant *as_constant = dynamic_cast<AstConstant*>(value_sp.get());
            if (as_constant != nullptr) {
                return as_constant->IsTrue();
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
