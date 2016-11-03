#include <ace-c/ast/ast_object.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/module.hpp>

#include <common/instructions.hpp>

#include <iostream>
#include <cassert>

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

    // load the type into a register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, static_id);

    // store newly allocated object in same register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint8_t>(NEW, rp, rp);

    // store object's register location
    const uint8_t obj_reg = rp;

    // claim register
    visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
    // get active register
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    // for each data member, load the default value
    // TODO: allow the user to assign values to items
    int i = 0;
    for (const DataMember_t &dm : m_object_type.GetDataMembers()) {
        assert(dm.second.GetDefaultValue() != nullptr && "default value was nullptr");

        // load the data member's default value.
        dm.second.GetDefaultValue()->Build(visitor, mod);
        // get active register
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
        // store data member
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(MOV_MEM, obj_reg, (uint8_t)i, rp);

        i++;
    }

    //unclaim register
    visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
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
