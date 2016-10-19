#include <athens/ast/ast_block.h>
#include <athens/ast_visitor.h>
#include <athens/emit/instruction.h>

#include <common/instructions.h>

AstBlock::AstBlock(const SourceLocation &location)
    : AstStatement(location),
      m_num_locals(0)
{
}

void AstBlock::Visit(AstVisitor *visitor)
{
    auto &mod = visitor->GetCompilationUnit()->CurrentModule();
    // open the new scope
    mod->m_scopes.Open(Scope());
    // visit all children in the block
    for (auto &child : m_children) {
        child->Visit(visitor);
    }

    // store number of locals, so we can pop them from the stack later
    Scope &this_scope = mod->m_scopes.Top();
    m_num_locals = this_scope.GetIdentifierTable().GetIdentifierIndex();

    // go down to previous scope
    visitor->GetCompilationUnit()->CurrentModule()->m_scopes.Close();
}

void AstBlock::Build(AstVisitor *visitor)
{
    for (std::shared_ptr<AstStatement> &stmt : m_children) {
        stmt->Build(visitor);
    }

    // pop all local variables off the stack
    for (int i = 0; i < m_num_locals; i++) {
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t>(POP);

        visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
    }
}

void AstBlock::Optimize(AstVisitor *visitor)
{
    for (auto &child : m_children) {
        child->Optimize(visitor);
    }
}
