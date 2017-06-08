#include <ace-c/ast/AstPrintStatement.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/emit/Instruction.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

AstPrintStatement::AstPrintStatement(const std::shared_ptr<AstArgumentList> &arg_list,
        const SourceLocation &location)
    : AstStatement(location),
      m_arg_list(arg_list)
{
}

void AstPrintStatement::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_arg_list != nullptr);
    m_arg_list->Visit(visitor, mod);
}

void AstPrintStatement::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_arg_list != nullptr);

    // accept each argument
    for (auto &arg : m_arg_list->GetArguments()) {
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
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    ASSERT(m_arg_list != nullptr);
    m_arg_list->Optimize(visitor, mod);
}

void AstPrintStatement::Recreate(std::ostringstream &ss)
{
    ss << Keyword::ToString(Keyword_print) << " ";

    ASSERT(m_arg_list != nullptr);
    m_arg_list->Recreate(ss);
}

Pointer<AstStatement> AstPrintStatement::Clone() const
{
    return CloneImpl();
}
