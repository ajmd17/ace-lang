#include <ace-c/ast/AstArrayExpression.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

#include <unordered_set>

AstArrayExpression::AstArrayExpression(const std::vector<std::shared_ptr<AstExpression>> &members,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_members(members),
      m_held_type(SymbolType::Builtin::ANY)
{
}

void AstArrayExpression::Visit(AstVisitor *visitor, Module *mod)
{
    std::unordered_set<SymbolTypePtr_t> held_types;

    for (auto &member : m_members) {
        ASSERT(member != nullptr);
        member->Visit(visitor, mod);

        if (member->GetSymbolType() != nullptr) {
            held_types.insert(member->GetSymbolType());
        } else {
            held_types.insert(SymbolType::Builtin::ANY);
        }
    }

    for (const auto &it : held_types) {
        ASSERT(it != nullptr);

        if (m_held_type == SymbolType::Builtin::UNDEFINED) {
            // `Undefined` invalidates the array type
            break;
        }
        
        if (m_held_type == SymbolType::Builtin::ANY) {
            // take first item found that is not `Any`
            m_held_type = it;
        } else if (m_held_type->TypeCompatible(*it, false)) {
            m_held_type = SymbolType::TypePromotion(m_held_type, it, true);
        } else {
            // more than one differing type, use Any.
            m_held_type = SymbolType::Builtin::ANY;
            break;
        }
    }
}

std::unique_ptr<Buildable> AstArrayExpression::Build(AstVisitor *visitor, Module *mod)
{
    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    bool has_side_effects = MayHaveSideEffects();
    uint32_t array_size = (uint32_t)m_members.size();
    
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    { // add NEW_ARRAY instruction
        auto instr_new_array = BytecodeUtil::Make<RawOperation<>>();
        instr_new_array->opcode = NEW_ARRAY;
        instr_new_array->Accept<uint8_t>(rp);
        instr_new_array->Accept<uint32_t>(array_size);
        chunk->Append(std::move(instr_new_array));
    }
    
    int stack_size_before = 0;

    if (has_side_effects) {
        // move to stack temporarily
        { // store value of the right hand side on the stack
            auto instr_push = BytecodeUtil::Make<RawOperation<>>();
            instr_push->opcode = PUSH;
            instr_push->Accept<uint8_t>(rp);
            chunk->Append(std::move(instr_push));
        }
        
        stack_size_before = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        // increment stack size
        visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();
    } else {
        // claim register for array
        visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();

        // get active register
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    }

    // assign all array items
    int index = 0;
    for (auto &member : m_members) {
        chunk->Append(member->Build(visitor, mod));

        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        if (has_side_effects) {
            // claim register for member
            visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
            // get active register
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            int stack_size_after = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
            int diff = stack_size_after - stack_size_before;
            ASSERT(diff == 1);

            { // load array from stack back into register
                auto instr_load_offset = BytecodeUtil::Make<RawOperation<>>();
                instr_load_offset->opcode = LOAD_OFFSET;
                instr_load_offset->Accept<uint8_t>(rp);
                instr_load_offset->Accept<uint16_t>(diff);
                chunk->Append(std::move(instr_load_offset));
            }

            { // send to the array
                auto instr_mov_array_idx = BytecodeUtil::Make<RawOperation<>>();
                instr_mov_array_idx->opcode = MOV_ARRAYIDX;
                instr_mov_array_idx->Accept<uint8_t>(rp);
                instr_mov_array_idx->Accept<uint32_t>(index);
                instr_mov_array_idx->Accept<uint8_t>(rp - 1);
                chunk->Append(std::move(instr_mov_array_idx));
            }

            // unclaim register for member
            visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
            // get active register
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
        } else {
            // send to the array
            auto instr_mov_array_idx = BytecodeUtil::Make<RawOperation<>>();
            instr_mov_array_idx->opcode = MOV_ARRAYIDX;
            instr_mov_array_idx->Accept<uint8_t>(rp - 1);
            instr_mov_array_idx->Accept<uint32_t>(index);
            instr_mov_array_idx->Accept<uint8_t>(rp);
            chunk->Append(std::move(instr_mov_array_idx));
        }

        index++;
    }

    if (!has_side_effects) {
        // unclaim register for array
        visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
        // get active register
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    } else {
        // move from stack to register 0
    
        int stack_size_after = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        int diff = stack_size_after - stack_size_before;
        ASSERT(diff == 1);
        
        { // load array from stack back into register
            auto instr_load_offset = BytecodeUtil::Make<RawOperation<>>();
            instr_load_offset->opcode = LOAD_OFFSET;
            instr_load_offset->Accept<uint8_t>(rp);
            instr_load_offset->Accept<uint16_t>(diff);
            chunk->Append(std::move(instr_load_offset));
        }

        { // pop the array from the stack
            auto instr_pop = BytecodeUtil::Make<RawOperation<>>();
            instr_pop->opcode = POP;
            chunk->Append(std::move(instr_pop));
        }

        // decrement stack size
        visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
    }

    return std::move(chunk);
}

void AstArrayExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    for (auto &member : m_members) {
        if (member != nullptr) {
            member->Optimize(visitor, mod);
        }
    }
}

void AstArrayExpression::Recreate(std::ostringstream &ss)
{
    ss << "[";
    for (auto &member : m_members) {
        if (member != nullptr) {
            member->Recreate(ss);
            ss << ",";
        }
    }
    ss << "]";
}

std::shared_ptr<AstStatement> AstArrayExpression::Clone() const
{
    return CloneImpl();
}

int AstArrayExpression::IsTrue() const
{
    return 1;
}

bool AstArrayExpression::MayHaveSideEffects() const
{
    bool side_effects = false;

    for (const auto &member : m_members) {
        ASSERT(member != nullptr);
        
        if (member->MayHaveSideEffects()) {
            side_effects = true;
            break;
        }
    }

    return side_effects;
}

SymbolTypePtr_t AstArrayExpression::GetSymbolType() const
{
    return SymbolType::GenericInstance(
        SymbolType::Builtin::ARRAY,
        GenericInstanceTypeInfo {
            {
                { "@array_of", m_held_type }
            }
        }
    );
}