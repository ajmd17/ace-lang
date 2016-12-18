#include <ace-c/ast/ast_function_call.hpp>
#include <ace-c/ast/ast_function_expression.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/ast/ast_constant.hpp>
#include <ace-c/emit/instruction.hpp>

#include <common/instructions.hpp>

#include <common/my_assert.hpp>
#include <iostream>

AstFunctionCall::AstFunctionCall(const std::string &name,
    const std::vector<std::shared_ptr<AstExpression>> &args,
    const SourceLocation &location)
    : AstIdentifier(name, location),
      m_args(args),
      m_return_type(ObjectType::type_builtin_any),
      m_has_self_object(false)
{
}

void AstFunctionCall::Visit(AstVisitor *visitor, Module *mod)
{
    AstIdentifier::Visit(visitor, mod);

    // visit each argument
    for (auto &arg : m_args) {
        ASSERT(arg != nullptr);
        arg->Visit(visitor, visitor->GetCompilationUnit()->GetCurrentModule().get());
    }
    
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
                    CompilerError(Level_fatal, Msg_closure_capture_must_be_parameter, m_location, m_name));
            }
        }
        
        const ObjectType &identifier_type = m_identifier->GetObjectType();
        if (identifier_type != ObjectType::type_builtin_any) {
            if (!identifier_type.IsFunctionType()) {
                // not a function type
                visitor->GetCompilationUnit()->GetErrorList().AddError(
                    CompilerError(Level_fatal, Msg_not_a_function, m_location, m_name));
            } else {
                // TODO: check parameters
                if (identifier_type.GetParamTypes().size() == m_args.size()) {
                    for (int i = 0; i < m_args.size(); i++) {
                        const ObjectType &param_type = identifier_type.GetParamTypes()[i];
                        if (param_type.HasTypeContract()) {
                            // make sure the argument of the function call satisfies the
                            // function's required type contract.
                            if (!param_type.GetTypeContract()->Satisfies(visitor, m_args[i]->GetObjectType())) {
                                // error, unsatisfied type contract
                                CompilerError error(Level_fatal, Msg_unsatisfied_type_contract, m_args[i]->GetLocation(), 
                                    m_args[i]->GetObjectType().ToString());
                                visitor->GetCompilationUnit()->GetErrorList().AddError(error);
                            }
                        }
                    }
                }

                ASSERT(identifier_type.GetReturnType() != nullptr);
                m_return_type = *identifier_type.GetReturnType().get();
            }
        }
    }
}

void AstFunctionCall::BuildArgumentsStart(AstVisitor *visitor, Module *mod)
{
    uint8_t rp;

    // push a copy of each argument to the stack
    for (auto &arg : m_args) {
        ASSERT(arg != nullptr);

        arg->Build(visitor, visitor->GetCompilationUnit()->GetCurrentModule().get());

        // get active register
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // now that it's loaded into the register, make a copy
        // add instruction to store on stack
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(PUSH, rp);

        // increment stack size
        visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();
    }

    // the reason we decrement the compiler's record of the stack size directly after
    // is because the function body will actually handle the management of the stack size,
    // so that the parameters are actually local variables to the function body.
    for (int i = 0; i < m_args.size(); i++) {
        // increment stack size
        visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
    }
}

void AstFunctionCall::BuildArgumentsEnd(AstVisitor *visitor, Module *mod)
{
    for (int i = 0; i < m_args.size(); i++) {
        // pop arguments from stack
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t>(POP);
    }
}

void AstFunctionCall::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_identifier != nullptr);

    BuildArgumentsStart(visitor, mod);

    int stack_size = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
    int stack_location = m_identifier->GetStackLocation();
    int offset = stack_size - stack_location + (int)m_args.size();

    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    if (!(m_identifier->GetFlags() & FLAG_DECLARED_IN_FUNCTION)) {
        // load globally, rather than from offset.
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_INDEX, rp, (uint16_t)stack_location);
    } else {
        // load stack value at offset value into register
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_OFFSET, rp, (uint16_t)offset);
    }

    // invoke the function
    int argc = (int)m_args.size();
    if (m_has_self_object) {
        argc++;
    }

    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint8_t>(CALL, rp, (uint8_t)argc);

    BuildArgumentsEnd(visitor, mod);
}

void AstFunctionCall::Optimize(AstVisitor *visitor, Module *mod)
{
    // optimize each argument
    for (auto &arg : m_args) {
        if (arg != nullptr) {
            arg->Optimize(visitor, visitor->GetCompilationUnit()->GetCurrentModule().get());
        }
    }
}

int AstFunctionCall::IsTrue() const
{
    // cannot deduce if return value is true
    return -1;
}

bool AstFunctionCall::MayHaveSideEffects() const
{
    // assume a function call has side effects
    // maybe we could detect this later
    return true;
}

ObjectType AstFunctionCall::GetObjectType() const
{
    return m_return_type;
}
