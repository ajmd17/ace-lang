#include <athens/ast/ast_variable_declaration.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/emit/instruction.hpp>
#include <athens/object_type.hpp>

#include <common/instructions.hpp>

AstVariableDeclaration::AstVariableDeclaration(const std::string &name,
    const std::shared_ptr<AstTypeSpecification> &type_specification,
    const std::shared_ptr<AstExpression> &assignment,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_type_specification(type_specification),
      m_assignment(assignment)
{
}

void AstVariableDeclaration::Visit(AstVisitor *visitor, Module *mod)
{
    ObjectType object_type;

    if (m_type_specification != nullptr) {
        m_type_specification->Visit(visitor, mod);
        object_type = m_type_specification->GetObjectType();
    }

    // if there was an assignment, visit it
    if (m_assignment != nullptr) {
        m_assignment->Visit(visitor, mod);

        // make sure type is compatible with assignment
        ObjectType assignment_type = m_assignment->GetObjectType();

        visitor->Assert(ObjectType::TypeCompatible(object_type, assignment_type, true),
            CompilerError(Level_fatal, Msg_mismatched_types, m_assignment->GetLocation(),
                object_type.ToString(), assignment_type.ToString()));
    }

    AstDeclaration::Visit(visitor, mod);

    m_identifier->SetObjectType(object_type);
}

void AstVariableDeclaration::Build(AstVisitor *visitor, Module *mod)
{
    if (m_identifier->GetUseCount() > 0) {
        if (m_assignment != nullptr) {
            // get current stack size
            int stack_location = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
            // set identifier stack location
            m_identifier->SetStackLocation(stack_location);

            m_assignment->Build(visitor, mod);

            // get active register
            uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            // add instruction to store on stack
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t>(PUSH, rp);

            // increment stack size
            visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();
        }
    }
}

void AstVariableDeclaration::Optimize(AstVisitor *visitor, Module *mod)
{
    if (m_assignment != nullptr) {
        m_assignment->Optimize(visitor, mod);
    }
}
