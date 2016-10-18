#include <athens/ast/ast_if_statement.h>
#include <athens/ast_visitor.h>
#include <athens/emit/instruction.h>
#include <athens/emit/static_object.h>

#include <common/instructions.h>

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

void AstIfStatement::Visit(AstVisitor *visitor)
{
    // visit the conditional
    m_conditional->Visit(visitor);
    // visit the body
    m_block->Visit(visitor);
    // visit the else-block (if it exists)
    if (m_else_block != nullptr) {
        m_else_block->Visit(visitor);
    }
}

void AstIfStatement::Build(AstVisitor *visitor)
{
    int condition_is_true = m_conditional->IsTrue();
    if (condition_is_true == -1) {
        // the condition cannot be determined at compile time
        uint8_t rp;

        // the label to jump to the very end
        StaticObject end_label;
        end_label.m_type = StaticObject::TYPE_LABEL;
        end_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

        // the label to jump to the else-part
        StaticObject else_label;
        else_label.m_type = StaticObject::TYPE_LABEL;
        else_label.m_id = (m_else_block != nullptr) ? visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId() : -1;

        // get current register index
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
        // build the conditional
        m_conditional->Build(visitor);
        // compare the conditional to 0
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(CMPZ, rp);
        // load the label address from static memory into register 0
        if (m_else_block != nullptr) {
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, else_label.m_id);
        } else {
            visitor->GetCompilationUnit()->GetInstructionStream() <<
                Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, end_label.m_id);
        }
        // jump if they are equal: i.e the value is false
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(JE, rp);

        // enter the block
        m_block->Build(visitor);
        // jump to the very end now that we've accepted the if-block
        visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
        // get current register index
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
        // load the label address from static memory into register 1
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, end_label.m_id);
        // jump if they are equal: i.e the value is false
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(JMP, rp);
        visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
        // get current register index
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        if (m_else_block != nullptr) {
            // set the label's position to where the else-block would be
            else_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
            visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(else_label);
            m_else_block->Build(visitor);
        }

        // set the label's position to after the block,
        // so we can skip it if the condition is false
        end_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
        visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(end_label);

    } else if (condition_is_true) {
        // the condition has been determined to be true
        // enter the block
        m_block->Build(visitor);
        // do not accept the else-block
    } else {
        // the condition has been determined to be false
        // only visit the else-block (if it exists)
        if (m_else_block != nullptr) {
            m_else_block->Build(visitor);
        }
    }
}

void AstIfStatement::Optimize(AstVisitor *visitor)
{
    // optimize the conditional
    m_conditional->Optimize(visitor);
    // optimize the body
    m_block->Optimize(visitor);
    // optimize the else-block (if it exists)
    if (m_else_block != nullptr) {
        m_else_block->Optimize(visitor);
    }
}
