#include <athens/ast/ast_try_catch.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/emit/instruction.hpp>
#include <athens/emit/static_object.hpp>

#include <common/instructions.hpp>

AstTryCatch::AstTryCatch(const std::shared_ptr<AstBlock> &try_block,
    const std::shared_ptr<AstBlock> &catch_block,
    const SourceLocation &location)
    : AstStatement(location),
      m_try_block(try_block),
      m_catch_block(catch_block)
{
}

void AstTryCatch::Visit(AstVisitor *visitor)
{
    // accept the try block
    m_try_block->Visit(visitor);
    // accept the catch block
    m_catch_block->Visit(visitor);
}

void AstTryCatch::Build(AstVisitor *visitor)
{
    uint8_t rp;

    // the label to jump to the very end
    StaticObject end_label;
    end_label.m_type = StaticObject::TYPE_LABEL;
    end_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

    // the label to jump to the catch-block
    StaticObject catch_label;
    catch_label.m_type = StaticObject::TYPE_LABEL;
    catch_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

    // get current register index
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, catch_label.m_id);

    // send the instruction to enter the try-block
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t>(BEGIN_TRY, rp);

    // build the try-block
    m_try_block->Build(visitor);

    // send the instruction to end the try-block
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t>(END_TRY);

    // jump to the end, as to not execute the catch-block
    // get current register index
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, end_label.m_id);

    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t>(JMP, rp);

    // set the label's position to where the catch-block would be
    catch_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
    visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(catch_label);

    // exception was thrown, pop all local variables from the try-block
    for (int i = 0; i < m_try_block->NumLocals(); i++) {
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t>(POP);
    }

    // build the catch-block
    m_catch_block->Build(visitor);

    end_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
    visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(end_label);
}

void AstTryCatch::Optimize(AstVisitor *visitor)
{
    // optimize the try block
    m_try_block->Optimize(visitor);
    // optimize the catch block
    m_catch_block->Optimize(visitor);
}
