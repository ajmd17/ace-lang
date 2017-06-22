#include <ace-c/ast/AstTupleExpression.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

#include <unordered_set>

AstTupleExpression::AstTupleExpression(const std::vector<std::shared_ptr<AstArgument>> &members,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_members(members)
{
}

void AstTupleExpression::Visit(AstVisitor *visitor, Module *mod)
{
    std::vector<SymbolMember_t> member_types;
    std::vector<GenericInstanceTypeInfo::Arg> generic_param_types;

    for (size_t i = 0; i < m_members.size(); i++) {
        const auto &member = m_members[i];
        ASSERT(member != nullptr);

        member->Visit(visitor, mod);

        std::string mem_name;
        
        if (member->IsNamed()) {
            mem_name = member->GetName();
        } else {
            mem_name = std::to_string(i);
        }

        // TODO find a better way to set up default assignment for members!
        // we can't  modify default values of types.
        //mem_type.SetDefaultValue(mem->GetAssignment());

        member_types.push_back(std::make_tuple(
            mem_name,
            member->GetSymbolType(),
            member->GetExpr()
        ));

        generic_param_types.push_back(GenericInstanceTypeInfo::Arg {
            mem_name,
            member->GetSymbolType(),
            nullptr
        });
    }

    m_symbol_type = SymbolType::Extend(SymbolType::GenericInstance(
        SymbolType::Builtin::TUPLE, 
        GenericInstanceTypeInfo {
            generic_param_types
        }
    ), member_types);
    
    // register the tuple type
    visitor->GetCompilationUnit()->RegisterType(m_symbol_type);
}

std::unique_ptr<Buildable> AstTupleExpression::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_symbol_type != nullptr);
    ASSERT(m_symbol_type->GetDefaultValue() != nullptr);

    return m_symbol_type->GetDefaultValue()->Build(visitor, mod);

    /*bool has_side_effects = MayHaveSideEffects();
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
        ASSERT(member != nullptr);

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
    }*/
}

void AstTupleExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    for (auto &member : m_members) {
        ASSERT(member != nullptr);

        member->Optimize(visitor, mod);
    }
}

void AstTupleExpression::Recreate(std::ostringstream &ss)
{
    ss << "(";
    for (auto &member : m_members) {
        ASSERT(member != nullptr);
        
        member->Recreate(ss);
        ss << ",";
    }
    ss << ")";
}

std::shared_ptr<AstStatement> AstTupleExpression::Clone() const
{
    return CloneImpl();
}

int AstTupleExpression::IsTrue() const
{
    return 1;
}

bool AstTupleExpression::MayHaveSideEffects() const
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

SymbolTypePtr_t AstTupleExpression::GetSymbolType() const
{
    ASSERT(m_symbol_type != nullptr);
    return m_symbol_type;
}