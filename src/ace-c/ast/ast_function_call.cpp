#include <ace-c/ast/ast_function_call.hpp>
#include <ace-c/ast/ast_function_expression.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/ast/ast_constant.hpp>
#include <ace-c/emit/instruction.hpp>

#include <common/instructions.hpp>

#include <cassert>

AstFunctionCall::AstFunctionCall(const std::string &name,
    const std::vector<std::shared_ptr<AstExpression>> &args,
    const SourceLocation &location)
    : AstIdentifier(name, location),
      m_args(args),
      m_return_type(ObjectType::type_builtin_undefined),
      m_has_self_object(false)
{
}

void AstFunctionCall::Visit(AstVisitor *visitor, Module *mod)
{
    AstIdentifier::Visit(visitor, mod);

    // visit each argument
    for (auto &arg : m_args) {
        assert(arg != nullptr);
        arg->Visit(visitor, visitor->GetCompilationUnit()->GetCurrentModule().get());
    }

    if (m_identifier != nullptr && m_identifier->GetObjectType() != ObjectType::type_builtin_any) {
        // make sure there are the right amount of arguments!
        auto value_sp = m_identifier->GetCurrentValue().lock();
        if (value_sp != nullptr) {
            AstFunctionExpression *def_sp = dynamic_cast<AstFunctionExpression*>(value_sp.get());
            if (def_sp == nullptr) {
                visitor->GetCompilationUnit()->GetErrorList().AddError(
                    CompilerError(Level_fatal, Msg_not_a_function, m_location, m_name));
            } else {
                // set return type.
                m_return_type = def_sp->GetReturnType();
            }
        }
    }
}

void AstFunctionCall::BuildArgumentsStart(AstVisitor *visitor, Module *mod)
{
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    // push a copy of each argument to the stack
    for (auto &arg : m_args) {
        assert(arg != nullptr);

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
    assert(m_identifier != nullptr);

    BuildArgumentsStart(visitor, mod);

    int stack_size = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
    int stack_location = m_identifier->GetStackLocation();
    int offset = stack_size - stack_location + m_args.size();

    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    if (m_in_function && !(m_identifier->GetFlags() & FLAG_DECLARED_IN_FUNCTION)) {
        // we must load globally, rather than from offset.
        // we are within a function right now, but loading a variable not declared in a function
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_GLOBAL, rp, (uint16_t)stack_location);
    } else {
        // load stack value at offset value into register
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_LOCAL, rp, (uint16_t)offset);
    }

    // invoke the function
    int argc = m_args.size();
    if (m_has_self_object) {
        argc++;
    }

    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint8_t>(CALL, rp, argc);

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
