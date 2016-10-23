#include <athens/ast/ast_function_definition.hpp>
#include <athens/emit/instruction.hpp>
#include <athens/emit/static_object.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/module.hpp>

#include <common/instructions.hpp>

AstFunctionDefinition::AstFunctionDefinition(const std::string &name,
    const std::vector<std::shared_ptr<AstParameter>> &parameters,
    const std::shared_ptr<AstBlock> &block,
    const SourceLocation &location)
    : AstDeclaration(name, location),
      m_parameters(parameters),
      m_block(block),
      m_static_id(0)
{
}

void AstFunctionDefinition::Visit(AstVisitor *visitor, Module *mod)
{
    // open the new scope for parameters
    mod->m_scopes.Open(Scope());

    for (auto &param : m_parameters) {
        if (param != nullptr) {
            // add the identifier to the table
            param->Visit(visitor, mod);
        }
    }

    // function body
    if (m_block != nullptr) {
        // visit the function body
        m_block->Visit(visitor, mod);
    }

    // close parameter scope
    mod->m_scopes.Close();

    AstDeclaration::Visit(visitor, mod);
}

void AstFunctionDefinition::Build(AstVisitor *visitor, Module *mod)
{
    if (m_identifier->GetUseCount() > 0) {
        // get current stack size
        int stack_location = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
        // set identifier stack location
        m_identifier->SetStackLocation(stack_location);

        // the properties of this function
        StaticFunction sf;
        sf.m_nargs = (uint8_t)m_parameters.size();

        // the register index variable we will reuse
        uint8_t rp;

        // the label to jump to the very end
        StaticObject end_label;
        end_label.m_type = StaticObject::TYPE_LABEL;
        end_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

        // jump to end as to not execute the function body
        // get current register index
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
        // load the label address from static memory into register
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, end_label.m_id);
        // jump if they are equal: i.e the value is false
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(JMP, rp);

        // store the function address before the function body
        sf.m_addr = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();

        // increase stack size by the number of parameters
        int param_stack_size = 0;
        for (auto &param : m_parameters) {
            if (param != nullptr) {
                param->Build(visitor, mod);
                param_stack_size++;
            }
        }

        if (m_block != nullptr) {
            // build the function body
            m_block->Build(visitor, mod);
        }

        // add RET instruction
        visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t>(RET);

        for (int i = 0; i < param_stack_size; i++) {
            visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
        }

        // set the label's position to after the block
        end_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
        visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(end_label);

        // store function data as a static object
        StaticObject so(sf);
        int found_id = visitor->GetCompilationUnit()->GetInstructionStream().FindStaticObject(so);
        if (found_id == -1) {
            m_static_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();
            so.m_id = m_static_id;
            visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(so);
        } else {
            m_static_id = found_id;
        }

        // store local variable
        // get register index
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // load the static object into register
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, m_static_id);

        // store on stack
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t>(PUSH, rp);

        // increment stack size
        visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();
    }
}

void AstFunctionDefinition::Optimize(AstVisitor *visitor, Module *mod)
{
    for (auto &param : m_parameters) {
        if (param != nullptr) {
            param->Optimize(visitor, mod);
        }
    }

    if (m_block != nullptr) {
        m_block->Optimize(visitor, mod);
    }
}
