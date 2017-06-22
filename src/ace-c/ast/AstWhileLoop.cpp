#include <ace-c/ast/AstWhileLoop.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/StaticObject.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/utf8.hpp>

#include <sstream>

AstWhileLoop::AstWhileLoop(const std::shared_ptr<AstExpression> &conditional,
    const std::shared_ptr<AstBlock> &block,
    const SourceLocation &location)
    : AstStatement(location),
      m_conditional(conditional),
      m_block(block),
      m_num_locals(0)
{
}

void AstWhileLoop::Visit(AstVisitor *visitor, Module *mod)
{
    // open scope
    mod->m_scopes.Open(Scope(SCOPE_TYPE_LOOP, 0));

    // visit the conditional
    m_conditional->Visit(visitor, mod);

    // visit the body
    m_block->Visit(visitor, mod);

    Scope &this_scope = mod->m_scopes.Top();
    m_num_locals = this_scope.GetIdentifierTable().CountUsedVariables();

    // close scope
    mod->m_scopes.Close();
}

std::unique_ptr<Buildable> AstWhileLoop::Build(AstVisitor *visitor, Module *mod)
{
    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    int condition_is_true = m_conditional->IsTrue();
    if (condition_is_true == -1) {
        // the condition cannot be determined at compile time
        uint8_t rp;

        LabelId top_label = chunk->NewLabel();

        // the label to jump to the end to BREAK
        LabelId break_label = chunk->NewLabel();

        // get current register index
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // where to jump up to
        chunk->MarkLabel(top_label);

        // build the conditional
        chunk->Append(m_conditional->Build(visitor, mod));

        { // compare the conditional to 0
            auto instr_cmpz = BytecodeUtil::Make<RawOperation<>>();
            instr_cmpz->opcode = CMPZ;
            instr_cmpz->Accept<uint8_t>(rp);
            chunk->Append(std::move(instr_cmpz));
        }

        { // break away if the condition is false (equal to zero)
            auto instr_je = BytecodeUtil::Make<Jump>();
            instr_je->opcode = JE;
            instr_je->label_id = break_label;
            chunk->Append(std::move(instr_je));
        }

        // enter the block
        chunk->Append(m_block->Build(visitor, mod));

        // pop all local variables off the stack
        for (int i = 0; i < m_num_locals; i++) {
            visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
        }

        chunk->Append(Compiler::PopStack(visitor, m_num_locals));

        { // jump back to top here
            auto instr_jmp = BytecodeUtil::Make<Jump>();
            instr_jmp->opcode = JMP;
            instr_jmp->label_id = top_label;
            chunk->Append(std::move(instr_jmp));
        }

        // set the label's position to after the block,
        // so we can skip it if the condition is false
        chunk->MarkLabel(break_label);
    } else if (condition_is_true) {
        LabelId top_label = chunk->NewLabel();
        chunk->MarkLabel(top_label);

        // the condition has been determined to be true
        if (m_conditional->MayHaveSideEffects()) {
            // if there is a possibility of side effects,
            // build the conditional into the binary
            chunk->Append(m_conditional->Build(visitor, mod));
        }

        // enter the block
        chunk->Append(m_block->Build(visitor, mod));

        // pop all local variables off the stack
        for (int i = 0; i < m_num_locals; i++) {
            visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
        }

        chunk->Append(Compiler::PopStack(visitor, m_num_locals));

        { // jump back to top here
            auto instr_jmp = BytecodeUtil::Make<Jump>();
            instr_jmp->opcode = JMP;
            instr_jmp->label_id = top_label;
            chunk->Append(std::move(instr_jmp));
        }
    } else {
        // the condition has been determined to be false
        if (m_conditional->MayHaveSideEffects()) {
            // if there is a possibility of side effects,
            // build the conditional into the binary
            chunk->Append(m_conditional->Build(visitor, mod));

            // pop all local variables off the stack
            for (int i = 0; i < m_num_locals; i++) {
                visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
            }

            chunk->Append(Compiler::PopStack(visitor, m_num_locals));
        }
    }

    return std::move(chunk);
}

void AstWhileLoop::Optimize(AstVisitor *visitor, Module *mod)
{
    // optimize the conditional
    m_conditional->Optimize(visitor, mod);
    // optimize the body
    m_block->Optimize(visitor, mod);
}

void AstWhileLoop::Recreate(std::ostringstream &ss)
{
    ASSERT(m_conditional != nullptr && m_block != nullptr);
    ss << Keyword::ToString(Keyword_while) << " ";
    m_conditional->Recreate(ss);
    m_block->Recreate(ss);
}

Pointer<AstStatement> AstWhileLoop::Clone() const
{
    return CloneImpl();
}
