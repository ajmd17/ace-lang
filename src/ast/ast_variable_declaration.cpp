#include <athens/ast/ast_variable_declaration.h>
#include <athens/ast_visitor.h>
#include <athens/emit/instruction.h>

#include <common/instructions.h>

AstVariableDeclaration::AstVariableDeclaration(const std::string &name, 
    const std::shared_ptr<AstExpression> &assignment,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_assignment(assignment)
{
}

void AstVariableDeclaration::Visit(AstVisitor *visitor)
{
    AstDeclaration::Visit(visitor);

    // if there was an assignment, visit it
    if (m_assignment != nullptr) {
        m_assignment->Visit(visitor);
    }
}

void AstVariableDeclaration::Build(AstVisitor *visitor)
{
    AstDeclaration::Build(visitor);

    if (m_identifier->GetUseCount() > 0) {
        if (m_assignment != nullptr) {
            m_assignment->Build(visitor);

            // get active register
            uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            // add instruction to store on stack
            visitor->GetCompilationUnit()->GetInstructionStream() << 
                Instruction<uint8_t, uint8_t>(PUSH, rp);
        }
    }
}

void AstVariableDeclaration::Optimize(AstVisitor *visitor)
{
    if (m_assignment != nullptr) {
        m_assignment->Optimize(visitor);
    }
}