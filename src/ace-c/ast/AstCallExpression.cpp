#include <ace-c/ast/AstCallExpression.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/ast/AstMember.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/emit/Instruction.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/utf8.hpp>

#include <limits>
#include <iostream>

AstCallExpression::AstCallExpression(
    const std::shared_ptr<AstExpression> &target,
    const std::vector<std::shared_ptr<AstArgument>> &args,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_target(target),
      m_args(args),
      m_return_type(SymbolType::Builtin::ANY),
      m_is_method_call(false)
{
}

void AstCallExpression::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);
    m_target->Visit(visitor, mod);

    SymbolTypePtr_t target_type = m_target->GetSymbolType();
    ASSERT(target_type != nullptr);

    // if the target is a member expression,
    // place it as 'self' argument to the call
    if (AstMember *target_mem = dynamic_cast<AstMember*>(m_target.get())) {
        m_is_method_call = true;

        std::shared_ptr<AstArgument> self_arg(new AstArgument(
            target_mem->GetTarget(),
            true,
            "self",
            m_target->GetLocation()
        ));
        
        // insert at front
        m_args.insert(m_args.begin(), self_arg);
    }

    // visit each argument
    for (auto &arg : m_args) {
        if (arg) {
            arg->Visit(visitor, mod);
        }
    }

    auto substituted = SemanticAnalyzer::SubstituteFunctionArgs(
        visitor, 
        mod,
        target_type,
        m_args,
        m_location
    );

    m_arg_ordering = substituted.second;
    m_return_type = substituted.first;

    if (m_return_type == nullptr) {
        // not a function type
        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
            Level_fatal,
            Msg_not_a_function,
            m_location,
            target_type->GetName()
        ));
    }
}

void AstCallExpression::BuildArgumentsStart(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_arg_ordering.size() >= m_args.size());

    uint8_t rp;

    // push a copy of each argument to the stack
    for (size_t i = 0; i < m_args.size(); i++) {
        ASSERT(m_arg_ordering[i] >= 0);
        ASSERT(m_arg_ordering[i] <= m_args.size());

        auto &arg = m_args[m_arg_ordering[i]];
        ASSERT(arg != nullptr);

        arg->Build(visitor, visitor->GetCompilationUnit()->GetCurrentModule());

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

void AstCallExpression::BuildArgumentsEnd(AstVisitor *visitor, Module *mod)
{
    // pop arguments from stack
    Compiler::PopStack(visitor, m_args.size());
}

void AstCallExpression::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);

    BuildArgumentsStart(visitor, mod);

    m_target->Build(visitor, mod);

    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    // invoke the function
    int argc = (int)m_args.size();

    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint8_t>(CALL, rp, (uint8_t)argc);

    BuildArgumentsEnd(visitor, mod);
}

void AstCallExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);

    m_target->Optimize(visitor, mod);

    // optimize each argument
    for (auto &arg : m_args) {
        if (arg) {
            arg->Optimize(visitor, visitor->GetCompilationUnit()->GetCurrentModule());
        }
    }
}

void AstCallExpression::Recreate(std::ostringstream &ss)
{
    ASSERT(m_target != nullptr);

    m_target->Recreate(ss);

    ss << "(";
    for (size_t i = 0; i < m_args.size(); i++) {
        auto &arg = m_args[i];
        if (arg) {
            arg->Recreate(ss);
            if (i != m_args.size() - 1) {
                ss << ",";
            }
        }
    }
    ss << ")";
}

Pointer<AstStatement> AstCallExpression::Clone() const
{
    return CloneImpl();
}

int AstCallExpression::IsTrue() const
{
    // cannot deduce if return value is true
    return -1;
}

bool AstCallExpression::MayHaveSideEffects() const
{
    // assume a function call has side effects
    // maybe we could detect this later
    return true;
}

SymbolTypePtr_t AstCallExpression::GetSymbolType() const
{
    return m_return_type;
}
