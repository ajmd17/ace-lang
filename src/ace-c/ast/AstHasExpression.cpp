#include <ace-c/ast/AstHasExpression.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Configuration.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

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

std::unique_ptr<Buildable> AstHasExpression::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);

    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    if (m_has_member != -1) {

      // get active register
      uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

      if (m_has_member == 1) {
          // load value into register
          chunk->Append(BytecodeUtil::Make<ConstBool>(rp, true));
      } else if (m_has_member == 0) {
          // load value into register
          chunk->Append(BytecodeUtil::Make<ConstBool>(rp, false));
      }

      // build in only if it has side effects
      if (m_target->MayHaveSideEffects()) {
          chunk->Append(m_target->Build(visitor, mod));
      }

    } else {
        // indeterminate at compile time.
        // check at runtime.
        const uint32_t hash = hash_fnv_1(m_field_name.c_str());

        int found_member_reg = -1;

        // the label to jump to the very end
        LabelId end_label = chunk->NewLabel();
        // the label to jump to the else-part
        LabelId else_label = chunk->NewLabel();
        
        chunk->Append(m_target->Build(visitor, mod));

        // get active register
        uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        { // compile in the instruction to check if it has the member
            auto instr_has_mem_hash = BytecodeUtil::Make<RawOperation<>>();
            instr_has_mem_hash->opcode = HAS_MEM_HASH;
            instr_has_mem_hash->Accept<uint8_t>(rp);
            instr_has_mem_hash->Accept<uint8_t>(rp);
            instr_has_mem_hash->Accept<uint32_t>(hash);
            chunk->Append(std::move(instr_has_mem_hash));
        }

        found_member_reg = rp;

        // compare the found member to zero
        chunk->Append(BytecodeUtil::Make<Comparison>(Comparison::CMPZ, found_member_reg));

        // jump if condition is false or zero.
        chunk->Append(BytecodeUtil::Make<Jump>(Jump::JE, else_label));
        
        // the member was found here, so load true
        chunk->Append(BytecodeUtil::Make<ConstBool>(rp, true));

        // jump to end after loading true
        chunk->Append(BytecodeUtil::Make<Jump>(Jump::JMP, end_label));

        chunk->MarkLabel(else_label);

        // member was not found, so load false
        chunk->Append(BytecodeUtil::Make<ConstBool>(rp, false));

        chunk->MarkLabel(end_label);
    }

    return std::move(chunk);
}

void AstHasExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_target != nullptr);

    m_target->Optimize(visitor, mod);
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