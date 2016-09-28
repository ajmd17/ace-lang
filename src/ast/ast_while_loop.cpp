#include <athens/ast/ast_while_loop.h>
#include <athens/ast_visitor.h>

AstWhileLoop::AstWhileLoop(const std::shared_ptr<AstExpression> &conditional, 
        const std::shared_ptr<AstBlock> &block,
        const SourceLocation &location)
    : AstStatement(location),
      m_conditional(conditional),
      m_block(block)
{
}

void AstWhileLoop::Visit(AstVisitor *visitor)
{
    // visit the conditional
    m_conditional->Visit(visitor);
    // visit the body
    m_block->Visit(visitor);
}

void AstWhileLoop::Build(AstVisitor *visitor) const
{
    int condition_is_true = m_conditional->IsTrue();
    if (condition_is_true == -1) {
        // the condition cannot be determined at compile time
    } else if (condition_is_true) {
        // the condition has been determined to be true
    } else {
        // the condition has been determined to be false
    }
}

void AstWhileLoop::Optimize()
{
    // optimize the conditional
    m_conditional->Optimize();
    // optimize the body
    m_block->Optimize();
}