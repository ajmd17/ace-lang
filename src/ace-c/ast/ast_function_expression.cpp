#include <ace-c/ast/ast_function_expression.hpp>
#include <ace-c/emit/instruction.hpp>
#include <ace-c/emit/static_object.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/module.hpp>
#include <ace-c/object_type.hpp>
#include <ace-c/configuration.hpp>

#include <common/instructions.hpp>

#include <vector>
#include <cassert>

AstFunctionExpression::AstFunctionExpression(const std::vector<std::shared_ptr<AstParameter>> &parameters,
    const std::shared_ptr<AstTypeSpecification> &type_specification,
    const std::shared_ptr<AstBlock> &block,
    const SourceLocation &location)
    : AstExpression(location),
      m_parameters(parameters),
      m_type_specification(type_specification),
      m_block(block),
      m_return_type(ObjectType::type_builtin_undefined),
      m_static_id(0)
{
}

void AstFunctionExpression::Visit(AstVisitor *visitor, Module *mod)
{
    std::vector<ObjectType> param_types;

    // open the new scope for parameters
    mod->m_scopes.Open(Scope(SCOPE_TYPE_FUNCTION));

    for (auto &param : m_parameters) {
        if (param != nullptr) {
            // add the identifier to the table
            param->Visit(visitor, mod);

            ObjectType param_type = ObjectType::type_builtin_undefined;

            // add the param's type to param_types
            if (param->GetIdentifier() != nullptr) {
                param_type = param->GetIdentifier()->GetObjectType();
                if (param->HasTypeContract()) {
                    param_type.SetTypeContract(param->GetTypeContract());
                }
            }

            param_types.push_back(param_type);
        }
    }

    // function body
    if (m_block != nullptr) {
        // visit the function body
        m_block->Visit(visitor, mod);
    }

    const Scope &function_scope = mod->m_scopes.Top();
    if (m_type_specification == nullptr) {
        // deduce return type.
        if (!function_scope.GetReturnTypes().empty()) {
            // search through return types for ambiguities
            for (const auto &it : function_scope.GetReturnTypes()) {
                if (m_return_type == ObjectType::type_builtin_any || 
                    m_return_type == ObjectType::type_builtin_undefined) {
                    m_return_type = it.first;
                } else if (ObjectType::TypeCompatible(m_return_type, it.first, false)) {
                    m_return_type = ObjectType::FindCompatibleType(m_return_type, it.first, true);
                } else {
                    // error; more than one possible return type.
                    visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(Level_fatal, Msg_multiple_return_types, it.second));
                }
            }
        } else {
            // return None.
            m_return_type = ObjectType::type_builtin_void;
        }
    } else {
        m_type_specification->Visit(visitor, mod);
        m_return_type = m_type_specification->GetObjectType();

        for (const auto &it : function_scope.GetReturnTypes()) {
            if (!ObjectType::TypeCompatible(m_return_type, it.first, true)) {
                // error; more than one possible return type.
                visitor->GetCompilationUnit()->GetErrorList().AddError(
                    CompilerError(Level_fatal, Msg_mismatched_return_type, it.second,
                        m_return_type.ToString(), it.first.ToString()));
            }
        }
    }
    // close parameter scope
    mod->m_scopes.Close();

    // set object type
    m_object_type = ObjectType::MakeFunctionType(m_return_type, param_types);
}

void AstFunctionExpression::Build(AstVisitor *visitor, Module *mod)
{
    assert(m_block != nullptr);

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

    if (!ace::compiler::Config::use_static_objects) {
        // fill with padding, for LOAD_ADDR instruction.
        visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
    }

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

    // build the function body
    m_block->Build(visitor, mod);

    if (!m_block->IsLastStatementReturn()) {
        // add RET instruction
        visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t>(RET);
    }

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

    if (!ace::compiler::Config::use_static_objects) {
        // fill with padding, for LOAD_FUNC instruction.
        visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 3;
    }
}

void AstFunctionExpression::Optimize(AstVisitor *visitor, Module *mod)
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

int AstFunctionExpression::IsTrue() const
{
    return 1;
}

bool AstFunctionExpression::MayHaveSideEffects() const
{
    return false;
}

ObjectType AstFunctionExpression::GetObjectType() const
{
    return m_object_type;
}
