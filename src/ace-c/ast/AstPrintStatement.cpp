#include <ace-c/ast/AstPrintStatement.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/emit/Instruction.hpp>

#include <common/instructions.hpp>

AstPrintStatement::AstPrintStatement(const std::vector<std::shared_ptr<AstExpression>> &arguments,
        const SourceLocation &location)
    : AstStatement(location),
      m_arguments(arguments)
{
}

void AstPrintStatement::Visit(AstVisitor *visitor, Module *mod)
{
    for (auto &arg : m_arguments) {
        if (arg) {
            arg->Visit(visitor, mod);
        }
    }
}

void AstPrintStatement::Build(AstVisitor *visitor, Module *mod)
{
    // accept each argument
    for (auto &arg : m_arguments) {
        arg->Build(visitor, mod);

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

void AstPrintStatement::Optimize(AstVisitor *visitor, Module *mod)
{
    for (auto &arg : m_arguments) {
        if (arg) {
            arg->Optimize(visitor, mod);
        }
    }
}

void AstPrintStatement::Recreate(std::ostringstream &ss)
{
    ss << Keyword::ToString(Keyword_print) << " ";
    for (auto &arg : m_arguments) {
        if (arg) {
            arg->Recreate(ss);
            ss << ",";
        }
    }
}