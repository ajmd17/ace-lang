#include <ace-c/ast/ast_while_loop.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/emit/instruction.hpp>
#include <ace-c/emit/static_object.hpp>
#include <ace-c/configuration.hpp>

#include <common/instructions.hpp>

AstWhileLoop::AstWhileLoop(const std::shared_ptr<AstExpression> &conditional,
        const std::shared_ptr<AstBlock> &block,
        const SourceLocation &location)
    : AstStatement(location),
      m_conditional(conditional),
      m_block(block)
{
}

void AstWhileLoop::Visit(AstVisitor *visitor, Module *mod)
{
    // open scope
    mod->m_scopes.Open(Scope(SCOPE_TYPE_LOOP));

    // visit the conditional
    m_conditional->Visit(visitor, mod);
    // visit the body
    m_block->Visit(visitor, mod);
    // close scope

    // close parameter scope
    mod->m_scopes.Close();
}

void AstWhileLoop::Build(AstVisitor *visitor, Module *mod)
{
    int condition_is_true = m_conditional->IsTrue();
    if (condition_is_true == -1) {
        // the condition cannot be determined at compile time
        uint8_t rp;

        StaticObject top_label;
        top_label.m_type = StaticObject::TYPE_LABEL;
        top_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

        // the label to jump to the end to BREAK
        StaticObject break_label;
        break_label.m_type = StaticObject::TYPE_LABEL;
        break_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

        // get current register index
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // where to jump up to
        top_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();

        // build the conditional
        m_conditional->Build(visitor, mod);
        // compare the conditional to 0
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(CMPZ, rp);

        // load the label address from static memory into register 0
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, break_label.m_id);

        if (!ace::compiler::Config::use_static_objects) {
            // fill with padding, for LOAD_ADDR instruction.
            visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
        }

        // jump if they are equal: i.e the value is false
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(JE, rp);

        // enter the block
        m_block->Build(visitor, mod);

        // get current register index
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
        // load the label address from static memory into register 0
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, top_label.m_id);
        if (!ace::compiler::Config::use_static_objects) {
            // fill with padding, for LOAD_ADDR instruction.
            visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
        }
        // jump back to top here
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(JMP, rp);

        // set the label's position to after the block,
        // so we can skip it if the condition is false
        break_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
        visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(break_label);
        visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(top_label);
    } else if (condition_is_true) {

        StaticObject top_label;
        top_label.m_type = StaticObject::TYPE_LABEL;
        top_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();
        top_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();

        // the condition has been determined to be true
        if (m_conditional->MayHaveSideEffects()) {
            // if there is a possibility of side effects,
            // build the conditional into the binary
            m_conditional->Build(visitor, mod);
        }
        // enter the block
        m_block->Build(visitor, mod);

        // get current register index
        uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
        // load the label address from static memory into register 0
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, top_label.m_id);
        if (!ace::compiler::Config::use_static_objects) {
            // fill with padding, for LOAD_ADDR instruction.
            visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
        }
        // jump back to top here
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(JMP, rp);

        visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(top_label);

    } else {
        // the condition has been determined to be false
        if (m_conditional->MayHaveSideEffects()) {
            // if there is a possibility of side effects,
            // build the conditional into the binary
            m_conditional->Build(visitor, mod);
        }
    }
}

void AstWhileLoop::Optimize(AstVisitor *visitor, Module *mod)
{
    // optimize the conditional
    m_conditional->Optimize(visitor, mod);
    // optimize the body
    m_block->Optimize(visitor, mod);
}
