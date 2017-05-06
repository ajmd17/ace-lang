#include <ace-c/ast/AstHasExpression.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/StaticObject.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/hasher.hpp>

#include <iostream>

AstHasExpression::AstHasExpression(
    const std::shared_ptr<AstExpression> &target,
    const std::string &field_name,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_target(target),
      m_field_name(field_name),
      m_has_member(-1)
{
}

void AstHasExpression::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);

    m_target->Visit(visitor, mod);

    SymbolTypePtr_t target_type = m_target->GetSymbolType();
    ASSERT(target_type != nullptr);

    if (target_type != SymbolType::Builtin::ANY) {
        if (SymbolTypePtr_t member_type = target_type->FindMember(m_field_name)) {
            m_has_member = 1;
        } else {
            m_has_member = 0;
        }
    } else {
        m_has_member = -1;
    }
}

void AstHasExpression::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);

    if (m_has_member != -1) {

      // get active register
      uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

      if (m_has_member == 1) {
          // load value into register
          visitor->GetCompilationUnit()->GetInstructionStream() <<
              Instruction<uint8_t, uint8_t>(LOAD_TRUE, rp);
      } else if (m_has_member == 0) {
          // load value into register
          visitor->GetCompilationUnit()->GetInstructionStream() <<
              Instruction<uint8_t, uint8_t>(LOAD_FALSE, rp);
      }

      // build in only if it has side effects
      if (m_target->MayHaveSideEffects()) {
          m_target->Build(visitor, mod);
      }

    } else {
        // indeterminate at compile time.
        // check at runtime.

        uint32_t hash = hash_fnv_1(m_field_name.c_str());

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

        // store the data in a register
        rp = visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();

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

        // enter the block
        // the member was found here, so load true

        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(LOAD_TRUE, rp);
        

        // unclaim register used to hold the object we're loading the member from
        visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();

        // this is the `else` part
        // jump to the very end now that we've accepted the if-block
        visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
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

        visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
        // get current register index
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // unclaim for conditional
        rp = visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();

        // set the label's position to where the else-block would be
        else_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
        visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(else_label);

        // member was not found, so load false
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(LOAD_FALSE, rp);

        // set the label's position to after the block,
        // so we can skip it if the condition is false
        end_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
        visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(end_label);
    }
}

void AstHasExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);

    m_target->Optimize(visitor, mod);
}

void AstHasExpression::Recreate(std::ostringstream &ss)
{
    ASSERT(m_target != nullptr);

    m_target->Recreate(ss);
    ss << " has \"" << m_field_name << "\"";
}

Pointer<AstStatement> AstHasExpression::Clone() const
{
    return CloneImpl();
}

SymbolTypePtr_t AstHasExpression::GetSymbolType() const
{
    return SymbolType::Builtin::BOOLEAN;
}

int AstHasExpression::IsTrue() const
{
    return m_has_member;
}

bool AstHasExpression::MayHaveSideEffects() const
{
    ASSERT(m_target != nullptr);

    return m_target->MayHaveSideEffects();
}