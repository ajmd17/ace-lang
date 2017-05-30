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
        if (member) {
            member->Visit(visitor, mod);
            held_types.insert(member->GetSymbolType());
        }
    }

    for (const auto &it : held_types) {
        if (it) {
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
}

void AstArrayExpression::Build(AstVisitor *visitor, Module *mod)
{
    bool has_side_effects = MayHaveSideEffects();
    uint32_t array_size = (uint32_t)m_members.size();
    
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    // create the new array with the size
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint32_t>(NEW_ARRAY, rp, array_size);

    
    int stack_size_before = 0;

    if (has_side_effects) {
        // move to stack temporarily
        
        // store value of the right hand side on the stack
        visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t, uint8_t>(PUSH, rp);
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

        member->Build(visitor, mod);
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        if (has_side_effects) {
            // claim register for member
            visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
            // get active register
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            int stack_size_after = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
            int diff = stack_size_after - stack_size_before;
            ASSERT(diff == 1);

            // load array from stack back into register
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t, uint16_t>(LOAD_OFFSET, rp, (uint16_t)diff);
                
            // send to the array
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t, uint32_t, uint8_t>(MOV_ARRAYIDX, rp, index, rp - 1);

            // unclaim register for member
            visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
            // get active register
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
        } else {
            // send to the array
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t, uint32_t, uint8_t>(MOV_ARRAYIDX, rp - 1, index, rp);
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
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_OFFSET, rp, (uint16_t)diff);

        // pop array from stack
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t>(POP);
        // decrement stack size
        visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
    }
}

void AstArrayExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    for (auto &member : m_members) {
        if (member) {
            member->Optimize(visitor, mod);
        }
    }
}

void AstArrayExpression::Recreate(std::ostringstream &ss)
{
    ss << "[";
    for (auto &member : m_members) {
        if (member) {
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
    for (auto &&member : m_members) {
        if (member->MayHaveSideEffects()) {
            side_effects = true;
            break;
        }
    }
    return side_effects;
}

SymbolTypePtr_t AstArrayExpression::GetSymbolType() const
{
    // TODO: determine the held type?
    return SymbolType::GenericInstance(
        SymbolType::Builtin::ARRAY,
        GenericInstanceTypeInfo {
            {
                { "@array_of", m_held_type }
            }
        }
    );
}