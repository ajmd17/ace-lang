#include <ace-c/compiler.hpp>
#include <ace-c/module.hpp>
#include <ace-c/ast/ast_module_declaration.hpp>

#include <common/instructions.hpp>

#include <cassert>

void Compiler::LoadLeftThenRight(AstVisitor *visitor, Module *mod, Compiler::ExprInfo info)
{
    // load left-hand side into register 0
    info.left->Build(visitor, mod);

    // right side has not been optimized away
    visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();

    if (info.right != nullptr) {
        // load right-hand side into register 1
        info.right->Build(visitor, mod);
    }
}

void Compiler::LoadRightThenLeft(AstVisitor *visitor, Module *mod, Compiler::ExprInfo info)
{
    uint8_t rp;

    // load right-hand side into register 0
    info.right->Build(visitor, mod);
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    bool left_side_effects = info.left->MayHaveSideEffects();

    // if left is a function call, we have to move rhs to the stack!
    // otherwise, the function call will overwrite what's in register 0.
    int stack_size_before;
    if (left_side_effects) {
        // store value of the right hand side on the stack
        visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t, uint8_t>(PUSH, rp);
        stack_size_before = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        // increment stack size
        visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();
    } else {
        visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
    }

    // load left-hand side into register 1
    info.left->Build(visitor, mod);

    if (left_side_effects) {
        // now, we increase register usage to load rhs from the stack into register 1.
        visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
        // get register position
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
        // load from stack
        int stack_size_after = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        int diff = stack_size_after - stack_size_before;
        assert(diff == 1);

        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_OFFSET, rp, (uint16_t)diff);
        // pop from stack
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t>(POP);
        // decrement stack size
        visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
    }
}

void Compiler::LoadLeftAndStore(AstVisitor *visitor, Module *mod, Compiler::ExprInfo info)
{
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    // load left-hand side into register 0
    info.left->Build(visitor, mod);
    // get register position
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // store value of lhs on the stack
    visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t, uint8_t>(PUSH, rp);

    int stack_size_before = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
    // increment stack size
    visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();

    // do NOT increase register usage (yet)
    // load right-hand side into register 0, overwriting previous lhs
    info.right->Build(visitor, mod);

    // now, we increase register usage to load lhs from the stack into register 1.
    visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
    // get register position
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    // load from stack
    int stack_size_after = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
    int diff = stack_size_after - stack_size_before;

    assert(diff == 1);

    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_OFFSET, rp, (uint16_t)diff);
    // pop from stack
    visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t>(POP);
    // decrement stack size
    visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
}

Compiler::Compiler(AstIterator *ast_iterator, CompilationUnit *compilation_unit)
    : AstVisitor(ast_iterator, compilation_unit)
{
}

Compiler::Compiler(const Compiler &other)
    : AstVisitor(other.m_ast_iterator, other.m_compilation_unit)
{
}

void Compiler::Compile(bool expect_module_decl)
{
    if (expect_module_decl) {
        if (m_ast_iterator->HasNext()) {
            auto first_statement = m_ast_iterator->Next();
            auto module_declaration = std::dynamic_pointer_cast<AstModuleDeclaration>(first_statement);

            if (module_declaration != nullptr) {
                // all files must begin with a module declaration
                module_declaration->Build(this, nullptr);
                CompileInner();
            }
        }
    } else {
        CompileInner();
    }
}

void Compiler::CompileInner()
{
    m_compilation_unit->m_module_index++;

    Module *mod = m_compilation_unit->m_modules[m_compilation_unit->m_module_index].get();

    while (m_ast_iterator->HasNext()) {
        m_ast_iterator->Next()->Build(this, mod);
    }

    // decrement the index to refer to the previous module
    m_compilation_unit->m_module_index--;
}
