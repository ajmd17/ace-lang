#include <athens/ast/ast_if_statement.h>
#include <athens/ast_visitor.h>
#include <athens/emit/instruction.h>
#include <athens/emit/static_object.h>

#include <common/instructions.h>

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

void AstIfStatement::Build(AstVisitor *visitor)
{
    int condition_is_true = m_conditional->IsTrue();
    if (condition_is_true == -1) {
        // the condition cannot be determined at compile time
        uint8_t rp;

        // the label for after this code block
        StaticObject after_label;
        after_label.m_type = StaticObject::TYPE_LABEL;
        after_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

        // build the conditional
        m_conditional->Build(visitor);

        visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
        // get current register index
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
        // load the value '0' (false) into register 1
        visitor->GetCompilationUnit()->GetInstructionStream() << 
            Instruction<uint8_t, uint8_t, int32_t>(LOAD_I32, rp, 0);
        // compare the conditional to 0
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint8_t>(
                CMP, rp - 1, rp);

        // unclaim the register
        visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
        // get current register index
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // load the label address from static memory into register 0
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, after_label.m_id);
        // jump if they are equal: i.e the value is false
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(JE, rp);

        // enter the block
        m_block->Build(visitor);

        // set the label's position to after the block,
        // so we can skip it if the condition is false
        after_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();

        visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(after_label);

    } else if (condition_is_true) {
        // the condition has been determined to be true

        // enter the block
        m_block->Build(visitor);
    } else {
        // the condition has been determined to be false
        // do nothing
    }
}

void AstIfStatement::Optimize(AstVisitor *visitor)
{
    // optimize the conditional
    m_conditional->Optimize(visitor);
    // optimize the body
    m_block->Optimize(visitor);
}