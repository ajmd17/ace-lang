#include <athens/ast/ast_print_statement.h>
#include <athens/ast_visitor.h>
#include <athens/emit/instruction.h>

#include <common/instructions.h>

AstPrintStatement::AstPrintStatement(const std::vector<std::shared_ptr<AstExpression>> &arguments,
        const SourceLocation &location)
    : AstStatement(location),
      m_arguments(arguments)
{
}

void AstPrintStatement::Visit(AstVisitor *visitor)
{
    for (auto &arg : m_arguments) {
        arg->Visit(visitor);
    }
}

void AstPrintStatement::Build(AstVisitor *visitor)
{
    // accept each argument
    for (auto &arg : m_arguments) {
        arg->Build(visitor);

        // get active register
        uint8_t rp = visitor->GetCompilationUnit()->
            GetInstructionStream().GetCurrentRegister();
        
        // emit instruction
        visitor->GetCompilationUnit()->GetInstructionStream() << 
            Instruction<uint8_t, uint8_t>(ECHO, rp);
    }

    // print newline
    visitor->GetCompilationUnit()->GetInstructionStream() << 
                Instruction<uint8_t>(ECHO_NEWLINE);
}

void AstPrintStatement::Optimize(AstVisitor *visitor)
{
    for (auto &arg : m_arguments) {
        arg->Optimize(visitor);
    }
}