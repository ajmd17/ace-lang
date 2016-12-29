#include <ace-c/ast/AstTryCatch.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/StaticObject.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

AstTryCatch::AstTryCatch(const std::shared_ptr<AstBlock> &try_block,
    const std::shared_ptr<AstBlock> &catch_block,
    const SourceLocation &location)
    : AstStatement(location),
      m_try_block(try_block),
      m_catch_block(catch_block)
{
}

void AstTryCatch::Visit(AstVisitor *visitor, Module *mod)
{
    // accept the try block
    m_try_block->Visit(visitor, mod);
    // accept the catch block
    m_catch_block->Visit(visitor, mod);
}

void AstTryCatch::Build(AstVisitor *visitor, Module *mod)
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

    if (!ace::compiler::Config::use_static_objects) {
        // fill with padding, for LOAD_ADDR instruction.
        visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
    }

    // send the instruction to enter the try-block
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t>(BEGIN_TRY, rp);

    // try block increases stack size to hold the data about the catch block
    visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();

    // build the try-block
    m_try_block->Build(visitor, mod);

    // send the instruction to end the try-block
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t>(END_TRY);

    // decrease stack size for the try block
    visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();

    // jump to the end, as to not execute the catch-block
    // get current register index
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, end_label.m_id);

    if (!ace::compiler::Config::use_static_objects) {
        // fill with padding, for LOAD_ADDR instruction.
        visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
    }

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
    m_catch_block->Build(visitor, mod);

    end_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
    visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(end_label);
}

void AstTryCatch::Optimize(AstVisitor *visitor, Module *mod)
{
    // optimize the try block
    m_try_block->Optimize(visitor, mod);
    // optimize the catch block
    m_catch_block->Optimize(visitor, mod);
}

void AstTryCatch::Recreate(std::ostringstream &ss)
{
    ASSERT(m_try_block != nullptr && m_catch_block != nullptr);
    
    ss << Keyword::ToString(Keyword_try);
    m_try_block->Recreate(ss);
    ss << Keyword::ToString(Keyword_catch);
    m_catch_block->Recreate(ss);
}