#include <ace-c/ast/ast_if_statement.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/compiler.hpp>
#include <ace-c/emit/instruction.hpp>
#include <ace-c/emit/static_object.hpp>

#include <common/instructions.hpp>

#include <cstdio>

AstIfStatement::AstIfStatement(const std::shared_ptr<AstExpression> &conditional,
        const std::shared_ptr<AstBlock> &block,
        const std::shared_ptr<AstBlock> &else_block,
        const SourceLocation &location)
    : AstStatement(location),
      m_conditional(conditional),
      m_block(block),
      m_else_block(else_block)
{
}

void AstIfStatement::Visit(AstVisitor *visitor, Module *mod)
{
    // visit the conditional
    m_conditional->Visit(visitor, mod);
    // visit the body
    m_block->Visit(visitor, mod);
    // visit the else-block (if it exists)
    if (m_else_block != nullptr) {
        m_else_block->Visit(visitor, mod);
    }
}

void AstIfStatement::Build(AstVisitor *visitor, Module *mod)
{
    int condition_is_true = m_conditional->IsTrue();
    if (condition_is_true == -1) {
        // the condition cannot be determined at compile time

        Compiler::CondInfo info {
            m_conditional.get(),
            m_block.get(),
            m_else_block.get()
        };

        Compiler::CreateConditional(visitor, mod, info);

    } else if (condition_is_true) {
        // the condition has been determined to be true
        if (m_conditional->MayHaveSideEffects()) {
            // if there is a possibility of side effects,
            // build the conditional into the binary
            m_conditional->Build(visitor, mod);
        }
        // enter the block
        m_block->Build(visitor, mod);
        // do not accept the else-block
    } else {
        // the condition has been determined to be false
        if (m_conditional->MayHaveSideEffects()) {
            // if there is a possibility of side effects,
            // build the conditional into the binary
            m_conditional->Build(visitor, mod);
        }
        // only visit the else-block (if it exists)
        if (m_else_block != nullptr) {
            m_else_block->Build(visitor, mod);
        }
    }
}

void AstIfStatement::Optimize(AstVisitor *visitor, Module *mod)
{
    // optimize the conditional
    m_conditional->Optimize(visitor, mod);
    // optimize the body
    m_block->Optimize(visitor, mod);
    // optimize the else-block (if it exists)
    if (m_else_block != nullptr) {
        m_else_block->Optimize(visitor, mod);
    }
}
