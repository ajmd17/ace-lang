#include <ace-c/ast/AstFunctionExpression.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/StaticObject.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/utf8.hpp>

#include <vector>
#include <iostream>

AstFunctionExpression::AstFunctionExpression(const std::vector<std::shared_ptr<AstParameter>> &parameters,
    const std::shared_ptr<AstTypeSpecification> &type_specification,
    const std::shared_ptr<AstBlock> &block,
    bool is_async,
    bool is_pure,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_parameters(parameters),
      m_type_specification(type_specification),
      m_block(block),
      m_is_async(is_async),
      m_is_pure(is_pure),
      m_return_type(SymbolType::Builtin::UNDEFINED),
      m_static_id(0)
{
}

void AstFunctionExpression::BuildFunctionBody(AstVisitor *visitor, Module *mod)
{
    // increase stack size by the number of parameters
    int param_stack_size = 0;
    for (auto &param : m_parameters) {
        if (param != nullptr) {
            param->Build(visitor, mod);
            param_stack_size++;
        }
    }

    // increase stack size for call stack info
    visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();

    // build the function body
    m_block->Build(visitor, mod);

    if (!m_block->IsLastStatementReturn()) {
        // add RET instruction
        visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t>(RET);
    }

    for (int i = 0; i < param_stack_size; i++) {
        visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
    }

    // decrease stack size for call stack info
    visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();

}

void AstFunctionExpression::Visit(AstVisitor *visitor, Module *mod)
{
    // first item will be return type
    std::vector<GenericInstanceTypeInfo::Arg> param_symbol_types;

    ScopeType scope_type = SCOPE_TYPE_FUNCTION;

    if (m_is_pure) {
        scope_type = SCOPE_TYPE_PURE_FUNCTION;
    }
    
    // open the new scope for parameters
    mod->m_scopes.Open(Scope(scope_type));

    for (auto &param : m_parameters) {
        if (param != nullptr) {
            // add the identifier to the table
            param->Visit(visitor, mod);

            SymbolTypePtr_t param_symbol_type = SymbolType::Builtin::UNDEFINED;
            // add the param's type to param_types
            
            if (param->GetIdentifier() != nullptr) {
                // add to list of param types
                param_symbol_types.push_back(GenericInstanceTypeInfo::Arg {
                    param->GetName(),
                    param->GetIdentifier()->GetSymbolType(),
                    param->GetDefaultValue()
                });
            }
        }
    }

    // function body
    if (m_block != nullptr) {
        // visit the function body
        m_block->Visit(visitor, mod);
    }

    const Scope &function_scope = mod->m_scopes.Top();

    // deduce return type
    if (!m_type_specification) {
        if (!function_scope.GetReturnTypes().empty()) {
            // search through return types for ambiguities
            for (const auto &it : function_scope.GetReturnTypes()) {
                if (m_return_type == SymbolType::Builtin::ANY || m_return_type == SymbolType::Builtin::UNDEFINED) {
                    m_return_type = it.first;
                } else if (m_return_type->TypeCompatible(*it.first, false)) {
                    m_return_type = SymbolType::TypePromotion(m_return_type, it.first, true);
                } else {
                    // error; more than one possible return type.
                    visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(LEVEL_ERROR, Msg_multiple_return_types, it.second)
                    );
                }
            }
        } else {
            // return null
            m_return_type = SymbolType::Builtin::ANY;
        }
    } else {
        m_type_specification->Visit(visitor, mod);
        m_return_type = m_type_specification->GetSymbolType();
        
        for (const auto &it : function_scope.GetReturnTypes()) {
            // use strict numbers because user specified return type
            if (!m_return_type->TypeCompatible(*it.first, true)) {
                // error; more than one possible return type.
                visitor->GetCompilationUnit()->GetErrorList().AddError(
                    CompilerError(
                        LEVEL_ERROR,
                        Msg_mismatched_return_type,
                        it.second,
                        m_return_type->GetName(),
                        it.first->GetName()
                    )
                );
            }
        }
    }

    // close parameter scope
    mod->m_scopes.Close();

    // set object type to be an instance of function
    std::vector<GenericInstanceTypeInfo::Arg> generic_param_types;
    generic_param_types.reserve(param_symbol_types.size() + 1);
    generic_param_types.push_back({
        "@return", m_return_type
    });

    for (auto &it : param_symbol_types) {
        generic_param_types.push_back(it);
    }

    m_symbol_type = SymbolType::GenericInstance(
        SymbolType::Builtin::FUNCTION, 
        GenericInstanceTypeInfo {
            generic_param_types
        }
    );
}

void AstFunctionExpression::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_block != nullptr);

    // the register index variable we will reuse
    uint8_t rp;

    if (!ace::compiler::Config::use_static_objects || m_static_id == 0) {

        // the properties of this function
        StaticFunction sf;
        sf.m_nargs = (uint8_t)m_parameters.size();
        sf.m_is_variadic = 0;
        
        if (!m_parameters.empty()) {
            const std::shared_ptr<AstParameter> &last = m_parameters.back();
            ASSERT(last != nullptr);

            sf.m_is_variadic = (uint8_t)last->IsVariadic();
        }

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

        // store function data as a static object
        StaticObject so(sf);
        int found_id = visitor->GetCompilationUnit()->GetInstructionStream().FindStaticObject(so);
        if (!ace::compiler::Config::use_static_objects || found_id == -1) {
            m_static_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();
            so.m_id = m_static_id;
            visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(so);
            

            // Build the function 
            BuildFunctionBody(visitor, mod);
        
        } else {
            m_static_id = found_id;
        }

        // set the label's position to after the block
        end_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
        visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(end_label);

    }

    // store local variable
    // get register index
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    // load the static object into register
    visitor->GetCompilationUnit()->GetInstructionStream() <<
        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, m_static_id);

    if (!ace::compiler::Config::use_static_objects) {
        // fill with padding, for LOAD_FUNC instruction.
        visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 4;
    }
}

void AstFunctionExpression::Optimize(AstVisitor *visitor, Module *mod)
{
    for (auto &param : m_parameters) {
        if (param) {
            param->Optimize(visitor, mod);
        }
    }

    if (m_block) {
        m_block->Optimize(visitor, mod);
    }
}

void AstFunctionExpression::Recreate(std::ostringstream &ss)
{
    ss << Keyword::ToString(Keyword_function);
    ss << "(";

    for (auto &param : m_parameters) {
        if (param) {
            param->Recreate(ss);
            ss << ",";
        }
    }

    ss << ")";

    if (m_block) {
        m_block->Recreate(ss);
    }
}

Pointer<AstStatement> AstFunctionExpression::Clone() const
{
    return CloneImpl();
}

int AstFunctionExpression::IsTrue() const
{
    return 1;
}

bool AstFunctionExpression::MayHaveSideEffects() const
{
    // changed to true because it affects registers
    return true;
}

SymbolTypePtr_t AstFunctionExpression::GetSymbolType() const
{
    return m_symbol_type;
}