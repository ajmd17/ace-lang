#include <ace-c/ast/AstFunctionExpression.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/StaticObject.hpp>
#include <ace-c/ast/AstArrayExpression.hpp>
#include <ace-c/ast/AstVariable.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Scope.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/utf8.hpp>
#include <common/hasher.hpp>

#include <vector>
#include <iostream>

AstFunctionExpression::AstFunctionExpression(
    const std::vector<std::shared_ptr<AstParameter>> &parameters,
    const std::shared_ptr<AstTypeSpecification> &type_specification,
    const std::shared_ptr<AstBlock> &block,
    bool is_async,
    bool is_pure,
    bool is_generator,
    const SourceLocation &location)
    : AstExpression(location, ACCESS_MODE_LOAD),
      m_parameters(parameters),
      m_type_specification(type_specification),
      m_block(block),
      m_is_async(is_async),
      m_is_pure(is_pure),
      m_is_generator(is_generator),
      m_is_closure(false),
      m_is_generator_closure(false),
      m_return_type(SymbolType::Builtin::UNDEFINED),
      m_static_id(0)
{
}

void AstFunctionExpression::BuildFunctionBody(AstVisitor *visitor, Module *mod)
{
    // increase stack size by the number of parameters
    int param_stack_size = 0;

    if (m_is_closure && m_closure_self_param != nullptr) {
        m_closure_self_param->Build(visitor, mod);
        param_stack_size++;
    }

    for (int i = 0; i < m_parameters.size(); i++) {
        ASSERT(m_parameters[i] != nullptr);
        m_parameters[i]->Build(visitor, mod);
        param_stack_size++;
    }

    // increase stack size for call stack info
    visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();

    if (m_is_generator) {
        ASSERT(m_generator_closure != nullptr);

        m_generator_closure->Build(visitor, mod);
        // return the generator closure object
        visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t>(RET);
    } else {
        // build the function body
        m_block->Build(visitor, mod);

        if (!m_block->IsLastStatementReturn()) {
            // add RET instruction
            visitor->GetCompilationUnit()->GetInstructionStream() << Instruction<uint8_t>(RET);
        }
    }

    for (int i = 0; i < param_stack_size; i++) {
        visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
    }

    // decrease stack size for call stack info
    visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
}

void AstFunctionExpression::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    // set m_is_closure to be true if we are already
    // located within a function
    m_is_closure = mod->IsInFunction();

    int scope_flags = 0;
    if (m_is_pure) {
        scope_flags |= ScopeFunctionFlags::PURE_FUNCTION_FLAG;
    }
    if (m_is_closure) {
        scope_flags |= ScopeFunctionFlags::CLOSURE_FUNCTION_FLAG;

        m_closure_self_param.reset(new AstParameter(
            "__closure_self",
            nullptr,
            nullptr,
            false,
            m_location
        ));
    }
    if (m_is_generator) {
        scope_flags |= ScopeFunctionFlags::GENERATOR_FUNCTION_FLAG;

        m_generator_closure.reset(new AstFunctionExpression(
            { std::shared_ptr<AstParameter>(new AstParameter(
                "__generator_callback",
                nullptr,
                nullptr,
                false,
                m_location
            )) },
            m_type_specification,
            m_block,
            m_is_async,
            m_is_pure,
            false,
            m_location
        ));

        m_generator_closure->SetIsGeneratorClosure(true);
    }
    
    // open the new scope for parameters
    mod->m_scopes.Open(Scope(
        SCOPE_TYPE_FUNCTION,
        scope_flags
    ));

    // first item will be set to return type
    std::vector<GenericInstanceTypeInfo::Arg> param_symbol_types;

    if (m_is_closure) {
        ASSERT(m_closure_self_param != nullptr);
        m_closure_self_param->Visit(visitor, mod);
    }

    for (auto &param : m_parameters) {
        if (param != nullptr) {
            // add the identifier to the table
            param->Visit(visitor, mod);
            
            ASSERT(param->GetIdentifier() != nullptr);
            // add to list of param types
            param_symbol_types.push_back(GenericInstanceTypeInfo::Arg {
                param->GetName(),
                param->GetIdentifier()->GetSymbolType(),
                param->GetDefaultValue()
            });
        }
    }

    if (m_is_generator) {
        ASSERT(m_generator_closure != nullptr);
        m_generator_closure->Visit(visitor, mod);

        ASSERT(m_generator_closure->GetSymbolType() != nullptr);

        m_return_type = m_generator_closure->GetSymbolType();
    } else {
        // function body
        if (m_block != nullptr) {
            // visit the function body
            m_block->Visit(visitor, mod);
        }

        if (m_type_specification != nullptr) {
            m_type_specification->Visit(visitor, mod);
            m_return_type = m_type_specification->GetSymbolType();
        }

        const Scope &function_scope = mod->m_scopes.Top();
        
        if (!function_scope.GetReturnTypes().empty()) {
            // search through return types for ambiguities
            for (const auto &it : function_scope.GetReturnTypes()) {
                ASSERT(it.first != nullptr);

                /* // check if this should be a generator
                if (it.first->GetTypeClass() == TYPE_GENERIC_INSTANCE) {
                    if (it.first->GetBaseType() != nullptr &&
                        it.first->GetBaseType()->TypeEqual(*SymbolType::Builtin::GENERATOR))
                    {
                        m_is_generator = true;
                    }
                } */

                if (m_type_specification != nullptr) {
                    // strict mode, because user specifically stated the intended return type
                    if (!m_return_type->TypeCompatible(*it.first, true)) {
                        // error; does not match what user specified
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
                } else {
                    // deduce return type
                    if (m_return_type == SymbolType::Builtin::ANY || m_return_type == SymbolType::Builtin::UNDEFINED) {
                        m_return_type = it.first;
                    } else if (m_return_type->TypeCompatible(*it.first, false)) {
                        m_return_type = SymbolType::TypePromotion(m_return_type, it.first, true);
                    } else {
                        // error; more than one possible deduced return type.
                        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                            LEVEL_ERROR,
                            Msg_multiple_return_types,
                            it.second
                        ));
                    }
                }
            }
        } else {
            // return null
            m_return_type = SymbolType::Builtin::ANY;
        }

    }
    
    // create data members to copy closure parameters
    std::vector<SymbolMember_t> closure_obj_members;

    const Scope &function_scope = mod->m_scopes.Top();

    for (const auto &it : function_scope.GetClosureCaptures()) {
        const std::string &name = it.first;
        const Identifier *ident = it.second;

        ASSERT(ident != nullptr);
        ASSERT(ident->GetSymbolType() != nullptr);

        std::shared_ptr<AstExpression> current_value(new AstVariable(
            name,
            m_location
        ));

        closure_obj_members.push_back({
            ident->GetName(),
            ident->GetSymbolType(),
            current_value
        });
    }

    // close parameter scope
    mod->m_scopes.Close();

    // set object type to be an instance of function
    std::vector<GenericInstanceTypeInfo::Arg> generic_param_types;
    generic_param_types.reserve(param_symbol_types.size() + 1);
    generic_param_types.push_back({
        "@return",
        m_return_type
    });

    if (m_is_closure) {
        ASSERT(m_closure_self_param != nullptr);
        ASSERT(m_closure_self_param->GetIdentifier() != nullptr);

        if (!closure_obj_members.empty() || m_closure_self_param->GetIdentifier()->GetUseCount() > 0) {
            generic_param_types.push_back(GenericInstanceTypeInfo::Arg {
                m_closure_self_param->GetName(),
                SymbolType::Builtin::ANY,
                nullptr
            });
        } else {
            // unset m_is_closure, as __closure_self param is unused.
            m_is_closure = false;
        }
    }
        
    for (auto &it : param_symbol_types) {
        generic_param_types.push_back(it);
    }

    m_symbol_type = SymbolType::GenericInstance(
        SymbolType::Builtin::FUNCTION, 
        GenericInstanceTypeInfo {
            generic_param_types
        }
    );

    if (m_is_closure) {
        closure_obj_members.push_back({
            "$invoke",
            m_symbol_type
        });

        for (auto &member : closure_obj_members) {
            if (std::get<2>(member) != nullptr) {
                std::get<2>(member)->Visit(visitor, mod);
            }
        }

        // create a new type for the 'events' field containing all listeners as fields
        m_closure_type = SymbolType::Object(
            "Closure",
            closure_obj_members
        );

        ASSERT(m_closure_type->GetDefaultValue() != nullptr);
        
        visitor->GetCompilationUnit()->RegisterType(m_closure_type);

        // builtin members:
        m_closure_object = m_closure_type->GetDefaultValue();
        m_closure_object->Visit(visitor, mod);
    }
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

        if (m_is_closure) {
            sf.m_nargs++; // make room for the closure self object
        }

        sf.m_flags = FunctionFlags::NONE;
        
        if (!m_parameters.empty()) {
            const std::shared_ptr<AstParameter> &last = m_parameters.back();
            ASSERT(last != nullptr);

            if (last->IsVariadic()) {
                sf.m_flags |= FunctionFlags::VARIADIC;
            }
        }

        if (m_is_generator_closure) {
            sf.m_flags |= FunctionFlags::GENERATOR;
        }

        if (m_is_closure) {
            sf.m_flags |= FunctionFlags::CLOSURE;
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
        const int found_id = visitor->GetCompilationUnit()->GetInstructionStream().FindStaticObject(so);

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

    if (m_is_closure) {
        ASSERT(m_closure_object != nullptr);

        const int func_expr_reg = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // increase reg usage for closure object to hold it while we move this function expr as a member
        visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        const int closure_obj_reg = rp;

        m_closure_object->Build(visitor, mod);

        const uint32_t hash = hash_fnv_1("$invoke");

        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint32_t, uint8_t>(MOV_MEM_HASH, rp, hash, (uint8_t)func_expr_reg);

        visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
        
        // swap regs, so the closure object returned
        visitor->GetCompilationUnit()->GetInstructionStream() <<
            Instruction<uint8_t, uint8_t, uint8_t>(MOV_REG, rp, (uint8_t)closure_obj_reg);
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

void AstFunctionExpression::Recreate(std::ostringstream &ss)
{
    ss << Keyword::ToString(Keyword_func);
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
    if (m_is_closure && m_closure_type != nullptr) {
        return m_closure_type;
    }
    return m_symbol_type;
}