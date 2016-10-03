#include <athens/ast/ast_if_statement.h>
#include <athens/ast_visitor.h>

#include <cstdio>

AstIfStatement::AstIfStatement(const std::shared_ptr<AstExpression> &conditional, 
        const std::shared_ptr<AstBlock> &block,
        const SourceLocation &location)
    : AstStatement(location),
      m_conditional(conditional),
      m_block(block)
{
}

void AstIfStatement::Visit(AstVisitor *visitor)
{
    // visit the conditional
    m_conditional->Visit(visitor);
    // visit the body
    m_block->Visit(visitor);
}

void AstIfStatement::Build(AstVisitor *visitor) const
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

void AstIfStatement::Optimize(AstVisitor *visitor)
{
    // optimize the conditional
    m_conditional->Optimize(visitor);
    // optimize the body
    m_block->Optimize(visitor);
}