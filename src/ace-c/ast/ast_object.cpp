#include <ace-c/ast/ast_object.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/module.hpp>
#include <ace-c/configuration.hpp>

#include <common/instructions.hpp>

#include <iostream>
#include <common/my_assert.hpp>

AstObject::AstObject(const ObjectType &object_type,
    const SourceLocation &location)
    : AstExpression(location),
      m_object_type(object_type)
{
}

void AstObject::Visit(AstVisitor *visitor, Module *mod)
{
}

void AstObject::Build(AstVisitor *visitor, Module *mod)
{
    int static_id = m_object_type.GetStaticId();

    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // store object's register location
    uint8_t obj_reg = rp;
    // store the original register location of the object
    const uint8_t original_obj_reg = obj_reg;

    // load the type into a register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, obj_reg, static_id);

    if (!ace::compiler::Config::use_static_objects) {
        // padding fill for LOAD_TYPE instruction!
        visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() +=
                m_object_type.GetDataMembers().size() * sizeof(uint32_t);
    }

    // store newly allocated object in same register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint8_t>(NEW, obj_reg, obj_reg);

    // store the type on the stack
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t>(PUSH, obj_reg);

    int obj_stack_loc = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
    // increment stack size for the type
    visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();

    // for each data member, load the default value
    int i = 0;
    for (const DataMember &dm : m_object_type.GetDataMembers()) {
        ASSERT_MSG(dm.m_type.GetDefaultValue() != nullptr, "default value was nullptr");

        if (dm.m_type.IsRecordType()) {
            // the member is a record type, and there is no chance of the 
            // register being overwritten,  so just load the type from obj_reg

            // claim register for the type
            visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
            // get active register
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            // load the data member's default value.
            dm.m_type.GetDefaultValue()->Build(visitor, mod);

            // get active register
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
            // store data member
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(MOV_MEM, obj_reg, (uint8_t)i, rp);
        } else {
            // if the member has side effects, load the type
            // from stack memory into a register /after/ loading the
            // data member.

            // build the data member
            dm.m_type.GetDefaultValue()->Build(visitor, mod);

            // claim register for the data member
            visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
            // get register position
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
            // set obj_reg for future data members
            obj_reg = rp;

            int stack_size = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
            int diff = stack_size - obj_stack_loc;
            ASSERT(diff == 1);

            // load type from stack
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t, uint16_t>(LOAD_OFFSET, obj_reg, (uint16_t)diff);

            // store data member
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(MOV_MEM, obj_reg, (uint8_t)i, rp - 1);
        }

        // unclaim register
        visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();

        i++;
    }

    // pop the type from stack
    visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t>(POP);
    // decrement stack size for type
    visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();

    // check if we have to move the register value back to the original location
    if (obj_reg != original_obj_reg) {
        // move the value in obj_reg into the original location
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint8_t>(MOV_REG, original_obj_reg, obj_reg);
    }
}

void AstObject::Optimize(AstVisitor *visitor, Module *mod)
{
}

int AstObject::IsTrue() const
{
    return 1;
}

bool AstObject::MayHaveSideEffects() const
{
    return false;
}

ObjectType AstObject::GetObjectType() const
{
    return m_object_type;
}
