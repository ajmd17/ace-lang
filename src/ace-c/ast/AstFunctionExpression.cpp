#include <ace-c/ast/AstFunctionExpression.hpp>
#include <ace-c/ast/AstArrayExpression.hpp>
#include <ace-c/ast/AstVariable.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Keywords.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Scope.hpp>
#include <ace-c/Configuration.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>

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
      m_return_type(BuiltinTypes::ANY),
      m_static_id(0)
{
}

std::unique_ptr<Buildable> AstFunctionExpression::BuildFunctionBody(AstVisitor *visitor, Module *mod)
{
    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    // increase stack size by the number of parameters
    int param_stack_size = 0;

    if (m_is_closure && m_closure_self_param != nullptr) {
        chunk->Append(m_closure_self_param->Build(visitor, mod));
        param_stack_size++;
    }

    for (int i = 0; i < m_parameters.size(); i++) {
        ASSERT(m_parameters[i] != nullptr);
        chunk->Append(m_parameters[i]->Build(visitor, mod));
        param_stack_size++;
    }

    // increase stack size for call stack info
    visitor->GetCompilationUnit()->GetInstructionStream().IncStackSize();

    if (m_is_generator) {
        ASSERT(m_generator_closure != nullptr);
        chunk->Append(m_generator_closure->Build(visitor, mod));

        // return the generator closure object
        chunk->Append(BytecodeUtil::Make<Return>());
    } else {
        // build the function body
        chunk->Append(m_block->Build(visitor, mod));

        if (!m_block->IsLastStatementReturn()) {
            // add RET instruction
            chunk->Append(BytecodeUtil::Make<Return>());
        }
    }

    for (int i = 0; i < param_stack_size; i++) {
        visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();
    }

    // decrease stack size for call stack info
    visitor->GetCompilationUnit()->GetInstructionStream().DecStackSize();

    return std::move(chunk);
}

void AstFunctionExpression::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr);
    ASSERT(mod != nullptr);

    // set m_is_closure to be true if we are already
    // located within a function
    m_is_closure = true;//mod->IsInFunction();

    int scope_flags = 0;
    
    if (m_is_pure) {
        scope_flags |= ScopeFunctionFlags::PURE_FUNCTION_FLAG;
    }

    if (m_is_closure) {
        scope_flags |= ScopeFunctionFlags::CLOSURE_FUNCTION_FLAG;

        // closures are objects with a method named '$invoke',
        // so we pass the 'self' argument when it is called.
        m_closure_self_param.reset(new AstParameter(
            "__closure_self",
            nullptr,
            nullptr,
            false,
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
                        it.first->GetBaseType()->TypeEqual(*BuiltinTypes::GENERATOR))
                    {
                        m_is_generator = true;
                    }
                } */

                if (m_type_specification != nullptr) {
                    // strict mode, because user specifically stated the intended return type
                    if (!m_return_type->TypeCompatible(*it.first, true)) {
                        // error; does not match what user specified
                        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                            LEVEL_ERROR,
                            Msg_mismatched_return_type,
                            it.second,
                            m_return_type->GetName(),
                            it.first->GetName()
                        ));
                    }
                } else {
                    // deduce return type
                    if (m_return_type == BuiltinTypes::ANY) {
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

                        break;
                    }
                }
            }
        } else {
            // return null
            m_return_type = BuiltinTypes::ANY;
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
        
        SymbolMember_t closure_member;
        std::get<0>(closure_member) = ident->GetName();
        std::get<1>(closure_member) = ident->GetSymbolType();
        std::get<2>(closure_member) = current_value;
        
        closure_obj_members.push_back(closure_member);
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

    // perform checking to see if it should still be considered a closure
    if (m_is_closure) {
        ASSERT(m_closure_self_param != nullptr);
        ASSERT(m_closure_self_param->GetIdentifier() != nullptr);

        if (!closure_obj_members.empty() || m_closure_self_param->GetIdentifier()->GetUseCount() > 0) {
            generic_param_types.push_back(GenericInstanceTypeInfo::Arg {
                m_closure_self_param->GetName(),
                BuiltinTypes::ANY,
                nullptr
            });
        } else {
            // unset m_is_closure, as closure 'self' param is unused.
            m_is_closure = false;
        }
    }
        
    for (auto &it : param_symbol_types) {
        generic_param_types.push_back(it);
    }

    m_symbol_type = SymbolType::GenericInstance(
        BuiltinTypes::FUNCTION, 
        GenericInstanceTypeInfo {
            generic_param_types
        }
    );

    if (m_is_closure) {
        // add $invoke to call this object
        SymbolMember_t invoke_member;
        std::get<0>(invoke_member) = "$invoke";
        std::get<1>(invoke_member) = m_symbol_type;
        closure_obj_members.push_back(invoke_member);

        // visit each member
        for (auto &member : closure_obj_members) {
            if (std::get<2>(member) != nullptr) {
                std::get<2>(member)->Visit(visitor, mod);
            }
        }

        SymbolTypePtr_t closure_base_type = SymbolType::GenericInstance(
            BuiltinTypes::CLOSURE_TYPE,
            GenericInstanceTypeInfo {
                generic_param_types
            }
        );

        visitor->GetCompilationUnit()->GetCurrentModule()->
            m_scopes.Root().GetIdentifierTable().AddSymbolType(closure_base_type);

        m_closure_type = SymbolType::Extend(closure_base_type, closure_obj_members);

        // register type
        visitor->GetCompilationUnit()->RegisterType(m_closure_type);

        // allow generic instance to be reused
        visitor->GetCompilationUnit()->GetCurrentModule()->
            m_scopes.Root().GetIdentifierTable().AddSymbolType(m_closure_type);

        ASSERT(m_closure_type->GetDefaultValue() != nullptr);

        m_closure_object = m_closure_type->GetDefaultValue();
        m_closure_object->Visit(visitor, mod);
    }
}

std::unique_ptr<Buildable> AstFunctionExpression::Build(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_block != nullptr);
    
    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    // the register index variable we will reuse
    uint8_t rp;

    // the properties of this function
    uint8_t nargs = (uint8_t)m_parameters.size();
    if (m_is_closure) {
        nargs++; // make room for the closure self object
    }

    uint8_t flags = FunctionFlags::NONE;
    if (!m_parameters.empty()) {
        const std::shared_ptr<AstParameter> &last = m_parameters.back();
        ASSERT(last != nullptr);

        if (last->IsVariadic()) {
            flags |= FunctionFlags::VARIADIC;
        }
    }

    if (m_is_generator_closure) {
        flags |= FunctionFlags::GENERATOR;
    }

    if (m_is_closure) {
        flags |= FunctionFlags::CLOSURE;
    }

    // the label to jump to the very end
    LabelId end_label = chunk->NewLabel();
    LabelId func_addr = chunk->NewLabel();

    // jump to end as to not execute the function body
    chunk->Append(BytecodeUtil::Make<Jump>(Jump::JMP, end_label));

    // store the function address before the function body
    chunk->Append(BytecodeUtil::Make<LabelMarker>(func_addr));
    
    // TODO add optimization to avoid duplicating the function body
    // Build the function 
    chunk->Append(BuildFunctionBody(visitor, mod));

    // set the label's position to after the block
    chunk->Append(BytecodeUtil::Make<LabelMarker>(end_label));

    // store local variable
    // get register index
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    auto func = BytecodeUtil::Make<BuildableFunction>();
    func->label_id = func_addr;
    func->reg = rp;
    func->nargs = nargs;
    func->flags = flags;
    chunk->Append(std::move(func));

    if (m_is_closure) {
        ASSERT(m_closure_object != nullptr);

        const int func_expr_reg = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        // increase reg usage for closure object to hold it while we move this function expr as a member
        visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

        const int closure_obj_reg = rp;

        chunk->Append(m_closure_object->Build(visitor, mod));

        const uint32_t hash = hash_fnv_1("$invoke");

        auto instr_mov_mem_hash = BytecodeUtil::Make<RawOperation<>>();
        instr_mov_mem_hash->opcode = MOV_MEM_HASH;
        instr_mov_mem_hash->Accept<uint8_t>(rp); // dst
        instr_mov_mem_hash->Accept<uint32_t>(hash); // hash
        instr_mov_mem_hash->Accept<uint8_t>(func_expr_reg); // src
        chunk->Append(std::move(instr_mov_mem_hash));
        
        visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();
        rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
        
        // swap regs, so the closure object returned
        auto instr_mov_reg = BytecodeUtil::Make<RawOperation<>>();
        instr_mov_reg->opcode = MOV_REG;
        instr_mov_reg->Accept<uint8_t>(rp); // dst
        instr_mov_reg->Accept<uint8_t>(closure_obj_reg); // src
        chunk->Append(std::move(instr_mov_reg));
    }

    return std::move(chunk);
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

Pointer<AstStatement> AstFunctionExpression::Clone() const
{
    return CloneImpl();
}

Tribool AstFunctionExpression::IsTrue() const
{
    return Tribool::True();
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
