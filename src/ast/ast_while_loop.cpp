#include <athens/ast/ast_while_loop.h>
#include <athens/ast_visitor.h>

AstWhileLoop::AstWhileLoop(std::unique_ptr<AstExpression> &&conditional, 
        std::unique_ptr<AstBlock> &&block,
        const SourceLocation &location)
    : AstStatement(location),
      m_conditional(std::move(conditional)),
      m_block(std::move(block))
{
}

void AstWhileLoop::Visit(AstVisitor *visitor)
{
    // visit the conditional
    m_conditional->Visit(visitor);
    // visit the body
    m_block->Visit(visitor);
}