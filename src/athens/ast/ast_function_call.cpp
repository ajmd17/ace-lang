#include <athens/ast/ast_function_call.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/ast/ast_constant.hpp>
#include <athens/emit/instruction.hpp>
#include <athens/emit/static_object.hpp>

#include <common/instructions.hpp>

AstFunctionCall::AstFunctionCall(const std::string &name,
        const std::vector<std::shared_ptr<AstExpression>> &args, const SourceLocation &location)
    : AstExpression(location),
      m_name(name),
      m_args(args),
      m_identifier(nullptr)
{
}

void AstFunctionCall::Visit(AstVisitor *visitor)
{
    // make sure that the variable exists
    std::unique_ptr<Module> &mod = visitor->GetCompilationUnit()->CurrentModule();
    Scope &scope = mod->m_scopes.Top();

    // the variable must exist in the active scope or a parent scope
    m_identifier = mod->LookUpIdentifier(m_name, false);
    if (m_identifier == nullptr) {
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_undeclared_identifier, m_location, m_name));
    } else {
        m_identifier->IncUseCount();
    }

    // visit each argument
    for (auto &arg : m_args) {
        if (arg != nullptr) {
            arg->Visit(visitor);
        }
    }
}

void AstFunctionCall::Build(AstVisitor *visitor)
{
    int stack_size = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
    int stack_location = m_identifier->GetStackLocation();
    int offset = stack_size - stack_location;

    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    int num_arguments = 0;

    // push a copy of each argument to the stack
    for (auto &arg : m_args) {
        if (arg != nullptr) {
            arg->Build(visitor);

            // get active register
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            // now that it's loaded into the register, make a copy
            // add instruction to store on stack
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t>(PUSH, rp);

            // increment stack size
            visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();

            offset++;

            num_arguments++;
        }
    }

    // the reason we decrement the compiler's record of the stack size directly after
    // is because the function body will actually handle the management of the stack size,
    // so that the parameters are actually local variables to the function body.
    for (int i = 0; i < num_arguments; i++) {
        // increment stack size
        visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
    }

    // get active register
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    // load stack value at offset value into register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_LOCAL, rp, (uint16_t)offset);

    // invoke the function
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint8_t>(CALL, rp, num_arguments);

    for (int i = 0; i < num_arguments; i++) {
        // pop arguments from stack
        visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t>(POP);
    }
}

void AstFunctionCall::Optimize(AstVisitor *visitor)
{
    // optimize each argument
    for (auto &arg : m_args) {
        if (arg != nullptr) {
            arg->Optimize(visitor);
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
