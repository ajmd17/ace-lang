#include <athens/ast/ast_variable_declaration.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/emit/instruction.hpp>

#include <common/instructions.hpp>

AstVariableDeclaration::AstVariableDeclaration(const std::string &name,
    const std::shared_ptr<AstExpression> &assignment,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_assignment(assignment)
{
}

void AstVariableDeclaration::Visit(AstVisitor *visitor, Module *mod)
{
    // if there was an assignment, visit it
    if (m_assignment != nullptr) {
        m_assignment->Visit(visitor, mod);
    }

    AstDeclaration::Visit(visitor, mod);
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
