#include <athens/ast/ast_block.h>
#include <athens/ast_visitor.h>

AstBlock::AstBlock(const SourceLocation &location)
    : AstStatement(location)
{
}

void AstBlock::Visit(AstVisitor *visitor)
{
    // open the new scope
    visitor->GetCompilationUnit()->CurrentModule()->m_scopes.Open(Scope());
    // visit all children in the block
    for (auto &child : m_children) {
        child->Visit(visitor);
    }
    // go down to previous scope
    visitor->GetCompilationUnit()->CurrentModule()->m_scopes.Close();
}

void AstBlock::Build(AstVisitor *visitor) const
{
}

void AstBlock::Optimize(AstVisitor *visitor)
{
    for (auto &child : m_children) {
        child->Optimize(visitor);
    }
}