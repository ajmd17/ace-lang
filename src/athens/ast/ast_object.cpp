#include <athens/ast/ast_object.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/module.hpp>

#include <common/instructions.hpp>

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
    // store newly allocated object in register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint16_t>(NEW, rp, static_id);
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
