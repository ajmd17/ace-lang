#include <ace-c/ast/AstBlock.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/ast/AstReturnStatement.hpp>

#include <common/instructions.hpp>

#include <limits>

AstBlock::AstBlock(const std::vector<std::shared_ptr<AstStatement>> &children, 
    const SourceLocation &location)
    : AstStatement(location),
      m_children(children),
      m_num_locals(0),
      m_last_is_return(false)
{
}

AstBlock::AstBlock(const SourceLocation &location)
    : AstStatement(location),
      m_num_locals(0),
      m_last_is_return(false)
{
}

void AstBlock::Visit(AstVisitor *visitor, Module *mod)
{
    // open the new scope
    mod->m_scopes.Open(Scope());

    // visit all children in the block
    for (auto &child : m_children) {
        if (child) {
            child->Visit(visitor, mod);
        }
    }

    m_last_is_return = (!m_children.empty()) &&
        (dynamic_cast<AstReturnStatement*>(m_children.back().get()) != nullptr);

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

    // how many times to pop the stack
    int pop_times = 0;

    // pop all local variables off the stack
    for (int i = 0; i < m_num_locals; i++) {
        if (!m_last_is_return) {
            pop_times++;
        }

        visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
    }

    Compiler::PopStack(visitor, pop_times);
}

void AstBlock::Optimize(AstVisitor *visitor, Module *mod)
{
    for (auto &child : m_children) {
        if (child) {
            child->Optimize(visitor, mod);
        }
    }
}

void AstBlock::Recreate(std::ostringstream &ss)
{
    ss << "{";
    for (size_t i = 0; i < m_children.size(); i++) {
        auto &child = m_children[i];
        if (child) {
            child->Recreate(ss);
            if (i != m_children.size() - 1) {
                ss << ";";
            }
        }
    }
    ss << "}";
}

Pointer<AstStatement> AstBlock::Clone() const
{
    return CloneImpl();
}
