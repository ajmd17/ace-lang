#include <ace-c/ast/ast_variable_declaration.hpp>
#include <ace-c/ast/ast_null.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/emit/instruction.hpp>
#include <ace-c/keywords.hpp>
#include <ace-c/object_type.hpp>
#include <ace-c/configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

#include <iostream>

AstVariableDeclaration::AstVariableDeclaration(const std::string &name,
    const std::shared_ptr<AstTypeSpecification> &type_specification,
    const std::shared_ptr<AstExpression> &assignment,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_type_specification(type_specification),
      m_assignment(assignment),
      m_assignment_already_visited(false)
{
}

void AstVariableDeclaration::Visit(AstVisitor *visitor, Module *mod)
{
    ObjectType object_type;

    if (m_type_specification == nullptr && m_assignment == nullptr) {
        // error; requires either type, or assignment.
        visitor->GetCompilationUnit()->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_missing_type_and_assignment, m_location, m_name));
    } else {
        if (m_type_specification != nullptr) {
            m_type_specification->Visit(visitor, mod);
            object_type = m_type_specification->GetObjectType();

            if (m_assignment == nullptr) {
                // Assign variable to the default value for the specified type.
                m_assignment = object_type.GetDefaultValue();
            }
        }

        ASSERT(m_assignment != nullptr);

        if (!m_assignment_already_visited) {
            // visit assignment
            m_assignment->Visit(visitor, mod);
        }

        // make sure type is compatible with assignment
        ObjectType assignment_type = m_assignment->GetObjectType();

        if (m_type_specification != nullptr) {
            visitor->Assert(ObjectType::TypeCompatible(object_type, assignment_type, true),
                CompilerError(Level_fatal, Msg_mismatched_types, m_assignment->GetLocation(),
                    object_type.ToString(), assignment_type.ToString()));
        } else {
            // Set the type to be the deduced type from the expression.
            object_type = assignment_type;
        }
    }

    AstDeclaration::Visit(visitor, mod);

    if (m_identifier != nullptr) {
        m_identifier->SetObjectType(object_type);
        m_identifier->SetCurrentValue(m_assignment);
    }
}

void AstVariableDeclaration::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_assignment != nullptr);
    if (!ace::compiler::Config::cull_unused_objects || m_identifier->GetUseCount() > 0) {
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
    } else {
        // if assignment has side effects but variable is unused,
        // compile the assignment in anyway.
        if (m_assignment->MayHaveSideEffects()) {
            m_assignment->Build(visitor, mod);
        }
    }
}

void AstVariableDeclaration::Optimize(AstVisitor *visitor, Module *mod)
{
    if (m_assignment) {
        m_assignment->Optimize(visitor, mod);
    }
}

void AstVariableDeclaration::Recreate(std::ostringstream &ss)
{
    if (m_assignment) {
        ss << Keyword::ToString(Keyword_let) << " ";
        ss << m_name << "=";
        m_assignment->Recreate(ss);
    } else if (m_type_specification) {
        ss << m_name << ":";
        m_type_specification->Recreate(ss);
    } else {
        ss << m_name << "=??";
    }
}