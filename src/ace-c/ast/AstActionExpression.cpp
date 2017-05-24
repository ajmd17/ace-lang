#include <ace-c/ast/AstActionExpression.hpp>
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

AstActionExpression::AstActionExpression(
    const std::string &action_name,
    const std::shared_ptr<AstExpression> &target,
    const std::vector<std::shared_ptr<AstArgument>> &args,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_action_name(action_name),
      m_target(target),
      m_args(args),
      m_return_type(SymbolType::Builtin::ANY),
      m_is_method_call(false)
{
}

void AstActionExpression::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);

    std::shared_ptr<AstArgument> self_arg(new AstArgument(
        m_target,
        true,
        "self",
        m_target->GetLocation()
    ));
    
    // insert self to front
    m_args.insert(m_args.begin(), self_arg);

    // build in a member access to get the objects 'events' field
    m_expr = std::shared_ptr<AstMember>(new AstMember(
      "events",
      m_target,
      m_location
    ));

    // build in check for events.<action name>
    m_expr = std::shared_ptr<AstMember>(new AstMember(
      m_action_name,
      m_expr,
      m_location
    ));

    // build in call
    m_expr = std::shared_ptr<AstCallExpression>(new AstCallExpression(
      m_expr,
      m_args,
      m_location
    ));

    m_expr->Visit(visitor, mod);
}

void AstActionExpression::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);
    m_expr->Build(visitor, mod);


   /* BuildArgumentsStart(visitor, mod);

    m_target->Build(visitor, mod);

    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    // invoke the function
    int argc = (int)m_args.size();

    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint8_t>(CALL, rp, (uint8_t)argc);

    BuildArgumentsEnd(visitor, mod);*/
}

void AstActionExpression::Optimize(AstVisitor *visitor, Module *mod)
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

void AstActionExpression::Recreate(std::ostringstream &ss)
{
    ASSERT(m_target != nullptr);

    m_target->Recreate(ss);

    ss << "<-";
    ss << "'" << m_action_name << "'";
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

Pointer<AstStatement> AstActionExpression::Clone() const
{
    return CloneImpl();
}

int AstActionExpression::IsTrue() const
{
    // cannot deduce if return value is true
    return -1;
}

bool AstActionExpression::MayHaveSideEffects() const
{
    // assume a function call has side effects
    // maybe we could detect this later
    return true;
}

SymbolTypePtr_t AstActionExpression::GetSymbolType() const
{
    return m_return_type;
}
