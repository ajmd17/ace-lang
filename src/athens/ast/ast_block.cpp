#include <athens/ast/ast_block.hpp>
#include <athens/emit/instruction.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/module.hpp>

#include <common/instructions.hpp>

AstBlock::AstBlock(const SourceLocation &location)
    : AstStatement(location),
      m_num_locals(0)
{
}

void AstBlock::Visit(AstVisitor *visitor, Module *mod)
{
    // open the new scope
    mod->m_scopes.Open(Scope());

    // visit all children in the block
    for (auto &child : m_children) {
        child->Visit(visitor, mod);
    }

    // store number of locals, so we can pop them from the stack later
    Scope &this_scope = mod->m_scopes.Top();
    m_num_locals = this_scope.GetIdentifierTable().CountUsedVariables();

    // go down to previous scope
    mod->m_scopes.Close();
}

void AstBlock::Build(AstVisitor *visitor, Module *mod)
{
    for (std::shared_ptr<AstStatement> &stmt : m_children) {
        stmt->Build(visitor, mod);
    }

    // pop all local variables off the stack
    for (int i = 0; i < m_num_locals; i++) {
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t>(POP);

        visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
    }
}

void AstBlock::Optimize(AstVisitor *visitor, Module *mod)
{
    for (auto &child : m_children) {
        child->Optimize(visitor, mod);
    }
}
