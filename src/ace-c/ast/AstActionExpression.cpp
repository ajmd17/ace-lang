#include <ace-c/ast/AstActionExpression.hpp>
#include <ace-c/ast/AstCallExpression.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/ast/AstMember.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/Configuration.hpp>

#include <common/hasher.hpp>
#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/utf8.hpp>

#include <limits>
#include <iostream>

AstActionExpression::AstActionExpression(
    const std::vector<std::shared_ptr<AstArgument>> &actions,
    const std::shared_ptr<AstExpression> &target,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_actions(actions),
      m_target(target),
      m_return_type(SymbolType::Builtin::ANY),
      m_is_method_call(false),
      m_member_found(-1)
{
}

void AstActionExpression::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(!m_actions.empty());
    //m_action->Visit(visitor, mod);

    ASSERT(m_target != nullptr);
    //m_target->Visit(visitor, mod);

    /*std::shared_ptr<AstArgument> self_arg(new AstArgument(
        m_target,
        true,
        "self",
        m_target->GetLocation()
    ));
    
    // insert self to front
    m_args.insert(m_args.begin(), self_arg);*/
    
    /*std::shared_ptr<AstArgument> action_arg((new AstArgument(
        m_action,
        false,
        "",
        SourceLocation::eof
    )));

    m_args.push_back(action_arg);*/

    std::shared_ptr<AstArgument> self_arg((new AstArgument(
        m_target,
        false,
        "",
        SourceLocation::eof
    )));

    m_actions.insert(
        m_actions.begin(),
        self_arg
    );

    // add events::get_action_handler call
    m_expr = visitor->GetCompilationUnit()->GetAstNodeBuilder()
        .Module("events")
        .Function("call_action")
        .Call(m_actions);

    /*m_expr = std::shared_ptr<AstCallExpression>(new AstCallExpression(
        m_expr,
        m_actions,
        false, // no 'self' - do not pass in object.events,
               // instead pass object (see above)
        m_location
    ));*/

    ASSERT(m_expr != nullptr);
    m_expr->Visit(visitor, mod);

    //SymbolTypePtr_t target_type = m_target->GetSymbolType();
    //ASSERT(target_type != nullptr);

    /*if (target_type != SymbolType::Builtin::ANY) {
        if (SymbolTypePtr_t member_type = target_type->FindMember("__events")) {
            m_member_found = 1;
        } else {
            m_member_found = 0;
        }
    } else {
        m_member_found = -1;
    }*/
}

void AstActionExpression::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);
    m_expr->Build(visitor, mod);
#if 0
    if (m_member_found == 1) {
        // found, build expr
        m_expr->Build(visitor, mod);
    } else if (m_member_found == -1) {
        // run-time check
        uint32_t hash = hash_fnv_1("__events");

        int found_member_reg = -1;

        // the label to jump to the very end
        StaticObject end_label;
        end_label.m_type = StaticObject::TYPE_LABEL;
        end_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

        // the label to jump to the else-part
        StaticObject else_label;
        else_label.m_type = StaticObject::TYPE_LABEL;
        else_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

        m_target->Build(visitor, mod);

        // get active register
        uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // compile in the instruction to check if it has the member
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint8_t, uint32_t>(HAS_MEM_HASH, rp, rp, hash);

        found_member_reg = rp;

        // compare the found member to zero
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(CMPZ, found_member_reg);

        // load the label address from static memory into register 0
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, (uint16_t)else_label.m_id);

        if (!ace::compiler::Config::use_static_objects) {
            // fill with padding, for LOAD_ADDR instruction.
            visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
        }

        // jump if condition is false or zero.
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(JE, rp);

        // not found here

        // this is the `else` part
        // jump to the very end now that we've accepted the if-block
        visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage(); // 1
        // get current register index
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // load the label address from static memory into register 1
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, end_label.m_id);

        if (!ace::compiler::Config::use_static_objects) {
            // fill with padding, for LOAD_ADDR instruction.
            visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
        }
        
        // jump if they are equal: i.e the value is false
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(JMP, rp);

        visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage(); // 0
        // get current register index
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // set the label's position to where the else-block would be
        else_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
        visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(else_label);

        // enter the block
        // the member was found here, so build the expr
        m_expr->Build(visitor, mod);

        // set the label's position to after the block,
        // so we can skip it if the condition is false
        end_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
        visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(end_label);
    }
#endif
    // re-build in the target. actions return their target after call
    //m_target->Build(visitor, mod);
}

void AstActionExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_expr != nullptr);
    m_expr->Optimize(visitor, mod);

   /* // optimize each argument
    for (auto &arg : m_args) {
        if (arg != nullptr) {
            arg->Optimize(visitor, visitor->GetCompilationUnit()->GetCurrentModule());
        }
    }*/
}

void AstActionExpression::Recreate(std::ostringstream &ss)
{
    /*ASSERT(m_action != nullptr);
    m_action->Recreate(ss);

    ss << "(";
    for (size_t i = 0; i < m_args.size(); i++) {
        auto &arg = m_args[i];
        if (arg != nullptr) {
            arg->Recreate(ss);
            if (i != m_args.size() - 1) {
                ss << ",";
            }
        }
    }
    ss << ")";

    ss << " => ";

    ASSERT(m_target != nullptr);
    m_target->Recreate(ss);*/
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
    return m_member_found != 0;
}

SymbolTypePtr_t AstActionExpression::GetSymbolType() const
{
    ASSERT(m_expr != nullptr);
    ASSERT(m_expr->GetSymbolType() != nullptr);

    return m_expr->GetSymbolType();
}
