#include <ace-c/Compiler.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/ast/AstModuleDeclaration.hpp>
#include <ace-c/ast/AstFunctionCall.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

void Compiler::LoadMemberFromHash(AstVisitor *visitor, Module *mod, uint32_t hash)
{
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint8_t, uint32_t>(LOAD_MEM_HASH, rp, rp, hash);
}

void Compiler::LoadMemberFromHashAndCall(AstVisitor *visitor, Module *mod,
    AstFunctionCall *field_as_call, uint32_t hash)
{
    uint8_t rp;

    int stack_size_before = 0;

    // push args
    int nargs = field_as_call->GetArguments().size();
    bool args_side_effects = false;

    // check if any args have side effects
    for (auto &arg : field_as_call->GetArguments()) {
        if (arg->MayHaveSideEffects()) {
            args_side_effects = true;
            break;
        }
    }

    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    // load data member.
    visitor->GetCompilationUnit()->GetInstructionStream() <<
       Instruction<uint8_t, uint8_t, uint8_t, uint32_t>(LOAD_MEM_HASH, rp, rp, hash);

    if (!args_side_effects) {
        // claim register for the loaded member
        rp = visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
    } else {
        // we have to push the member to the stack

        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(PUSH, rp);

        stack_size_before = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        // increment stack size
        visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();
    }

    field_as_call->BuildArgumentsStart(visitor, mod);

    if (!args_side_effects) {
        // unclaim register for the member
        rp = visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
    } else {
        // get register usage
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // load from stack
        int stack_size_after = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        // we add nargs because the stack size would have increased by pushing the arguments
        int diff = stack_size_after - stack_size_before + nargs;

        ASSERT(diff == nargs + 1);

        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_OFFSET, rp, (uint16_t)diff);
    }

    // invoke the member
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint8_t>(CALL, rp, (uint8_t)nargs);

    // pop args
    field_as_call->BuildArgumentsEnd(visitor, mod);

    // pop the member from the stack
    if (args_side_effects) {
        // pop from stack
        visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t>(POP);

        // decrement stack size
        visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
    }
}

void Compiler::LoadMemberAtIndex(AstVisitor *visitor, Module *mod, int dm_index)
{
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(LOAD_MEM, rp, rp, (uint8_t)dm_index);
}

void Compiler::StoreMemberAtIndex(AstVisitor *visitor, Module *mod, int dm_index)
{
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(MOV_MEM, rp, (uint8_t)dm_index, rp - 1);
}

void Compiler::LoadMemberAtIndexAndCall(AstVisitor *visitor, Module *mod,
    AstFunctionCall *field_as_call, int dm_index)
{
    uint8_t rp;

    int stack_size_before = 0;

    // push args
    int nargs = field_as_call->GetArguments().size();
    bool args_side_effects = false;

    // check if any args have side effects
    for (auto &arg : field_as_call->GetArguments()) {
        if (arg->MayHaveSideEffects()) {
            args_side_effects = true;
            break;
        }
    }

    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    // load data member.
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(LOAD_MEM, rp, rp, (uint8_t)dm_index);

    if (!args_side_effects) {
        // claim register for the loaded member
        rp = visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
    } else {
        // we have to push the member to the stack

        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(PUSH, rp);

        stack_size_before = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        // increment stack size
        visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();
    }

    field_as_call->BuildArgumentsStart(visitor, mod);

    if (!args_side_effects) {
        // unclaim register for the member
        rp = visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
    } else {
        // get register usage
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // load from stack
        int stack_size_after = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        // we add nargs because the stack size would have increased by pushing the arguments
        int diff = stack_size_after - stack_size_before + nargs;

        ASSERT(diff == nargs + 1);

        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_OFFSET, rp, (uint16_t)diff);
    }

    // invoke the member
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint8_t>(CALL, rp, (uint8_t)nargs);

    // pop args
    field_as_call->BuildArgumentsEnd(visitor, mod);

    // pop the member from the stack
    if (args_side_effects) {
        // pop from stack
        visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t>(POP);

        // decrement stack size
        visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
    }
}

void Compiler::BuildUCS(AstVisitor *visitor, Module *mod, AstFunctionCall *field_as_call)
{
    // allows functions to be used like they are
    // members (uniform call syntax)
    // in this case it would be usage of uniform call syntax.

    // build what we have so far into the function
    // get active register
    uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    // make a copy of the data we already got from the member access
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t>(PUSH, rp);

    // increment stack size
    visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();

    // build the function call
    field_as_call->SetHasSelfObject(true);
    field_as_call->Build(visitor, mod);

    // decrement stack size
    visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();

    // pop argument from stack
    visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t>(POP);
}

void Compiler::CreateConditional(AstVisitor *visitor, Module *mod, CondInfo info)
{
    ASSERT(info.cond != nullptr && info.then_part != nullptr);

    uint8_t rp;

    // the label to jump to the very end
    StaticObject end_label;
    end_label.m_type = StaticObject::TYPE_LABEL;
    end_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

    // the label to jump to the else-part
    StaticObject else_label;
    else_label.m_type = StaticObject::TYPE_LABEL;
    else_label.m_id = (info.else_part != nullptr) ? visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId() : -1;

    // get current register index
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
    // build the conditional
    info.cond->Build(visitor, mod);
    // compare the conditional to 0
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t>(CMPZ, rp);

    // get current register index
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    // load the label address from static memory into register 0
    if (info.else_part != nullptr) {
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, (uint16_t)else_label.m_id);

        if (!ace::compiler::Config::use_static_objects) {
            // fill with padding, for LOAD_ADDR instruction.
            visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
        }
    } else {
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, (uint16_t)end_label.m_id);

        if (!ace::compiler::Config::use_static_objects) {
            // fill with padding, for LOAD_ADDR instruction.
            visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
        }
    }

    // jump if condition is false or zero.
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t>(JE, rp);

    // enter the block
    info.then_part->Build(visitor, mod);

    if (info.else_part != nullptr) {
        // jump to the very end now that we've accepted the if-block
        visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
        // get current register index
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // load the label address from static memory into register 1
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, end_label.m_id);

        if (!ace::compiler::Config::use_static_objects) {
            // fill with padding, for LOAD_ADDR instruction.
            visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
        }
        // jump if they are equal: i.e the value is false
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(JMP, rp);
        visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
        // get current register index
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // set the label's position to where the else-block would be
        else_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
        visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(else_label);
        info.else_part->Build(visitor, mod);
    }

    // set the label's position to after the block,
    // so we can skip it if the condition is false
    end_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
    visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(end_label);
}

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
    int stack_size_before = 0;
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
        ASSERT(diff == 1);

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
    uint8_t rp;

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

    ASSERT(diff == 1);

    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_OFFSET, rp, (uint16_t)diff);
    // pop from stack
    visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t>(POP);
    // decrement stack size
    visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
}

void Compiler::PopStack(AstVisitor *visitor, int amt)
{
    if (amt == 1) {
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t>(POP);
    } else {
        for (int i = 0; i < amt;) {
            int j = 0;
            while (j < std::numeric_limits<uint8_t>::max() && i < amt) {
                j++, i++;
            }
            
            if (j > 0) {
                ASSERT(j <= std::numeric_limits<uint8_t>::max());
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t>(POP_N, (uint8_t)j);
            }
        }
    }
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

            if (module_declaration) {
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
    Module *mod = m_compilation_unit->GetCurrentModule();
    while (m_ast_iterator->HasNext()) {
        m_ast_iterator->Next()->Build(this, mod);
    }
}