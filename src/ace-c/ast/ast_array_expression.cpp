#include <ace-c/ast/ast_array_expression.hpp>
#include <ace-c/emit/instruction.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/module.hpp>
#include <ace-c/object_type.hpp>
#include <ace-c/configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

AstArrayExpression::AstArrayExpression(const std::vector<std::shared_ptr<AstExpression>> &members,
    const SourceLocation &location)
    : AstExpression(location),
      m_members(members)
{
}

void AstArrayExpression::Visit(AstVisitor *visitor, Module *mod)
{
    for (auto &member : m_members) {
        member->Visit(visitor, mod);
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
        member->Optimize(visitor, mod);
    }
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

ObjectType AstArrayExpression::GetObjectType() const
{
    // TODO: determine the held type?
    return ObjectType::MakeArrayType(ObjectType::type_builtin_any);
}