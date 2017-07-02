#include <ace-c/ast/AstCallExpression.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/ast/AstMember.hpp>
#include <ace-c/SemanticAnalyzer.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/utf8.hpp>

#include <limits>
#include <iostream>

AstCallExpression::AstCallExpression(
    const std::shared_ptr<AstExpression> &target,
    const std::vector<std::shared_ptr<AstArgument>> &args,
    bool insert_self,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_target(target),
      m_args(args),
      m_insert_self(insert_self),
      m_return_type(SymbolType::Builtin::UNDEFINED),
      m_is_method_call(false)
{
}

void AstCallExpression::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);
    m_target->Visit(visitor, mod);

    SymbolTypePtr_t target_type = m_target->GetSymbolType();
    ASSERT(target_type != nullptr);

    if (m_insert_self) {
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
    }

    if (SymbolTypePtr_t call_member_type = target_type->FindMember("$invoke")) {
        m_is_method_call = true;

        // closure objects have a self parameter for the '$invoke' call.
        std::shared_ptr<AstArgument> self_arg(new AstArgument(
            m_target,
            false,
            "__closure_self",
            m_target->GetLocation()
        ));
        
        // insert at front
        m_args.insert(m_args.begin(), self_arg);

        m_target.reset(new AstMember(
            "$invoke",
            m_target,
            m_location
        ));
        
        ASSERT(m_target != nullptr);
        m_target->Visit(visitor, mod);

        target_type = call_member_type;
        ASSERT(target_type != nullptr);
    }

    // visit each argument
    for (auto &arg : m_args) {
        ASSERT(arg != nullptr);

        // note, visit in current module rather than module access
        // this is used so that we can call functions from separate modules,
        // yet still pass variables from the local module.
        arg->Visit(visitor, visitor->GetCompilationUnit()->GetCurrentModule());
    }

    auto substituted = SemanticAnalyzer::Helpers::SubstituteFunctionArgs(
        visitor, 
        mod,
        target_type,
        m_args,
        m_location
    );

    // change args to be newly ordered vector
    m_args = substituted.second;

    if (substituted.first == nullptr) {
        // not a function type
        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
            LEVEL_ERROR,
            Msg_not_a_function,
            m_location,
            target_type->GetName()
        ));
    } else {
        m_return_type = substituted.first;
    }
}

std::unique_ptr<Buildable> AstCallExpression::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);

    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    // build arguments
    chunk->Append(Compiler::BuildArgumentsStart(
        visitor,
        mod,
        m_args//args_sorted
    ));

    chunk->Append(Compiler::BuildCall(
        visitor,
        mod,
        m_target,
        (uint8_t)m_args.size()
    ));

    chunk->Append(Compiler::BuildArgumentsEnd(
        visitor,
        mod,
        m_args.size()
    ));

    return std::move(chunk);
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
    ASSERT(m_return_type != nullptr);
    return m_return_type;
}
