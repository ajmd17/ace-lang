#include <ace-c/ast/AstVariable.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/ast/AstConstant.hpp>
#include <ace-c/ast/AstInteger.hpp>
#include <ace-c/Scope.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>
#include <ace-c/emit/StorageOperation.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

#include <iostream>

AstVariable::AstVariable(const std::string &name, const SourceLocation &location)
    : AstIdentifier(name, location)
{
}

void AstVariable::Visit(AstVisitor *visitor, Module *mod)
{
    AstIdentifier::Visit(visitor, mod);

    ASSERT(m_properties.GetIdentifierType() != IDENTIFIER_TYPE_UNKNOWN);

    switch (m_properties.GetIdentifierType()) {
        case IDENTIFIER_TYPE_VARIABLE: {
            ASSERT(m_properties.GetIdentifier() != nullptr);

            if (m_properties.GetIdentifier()->GetFlags() & IdentifierFlags::FLAG_ALIAS) {
                const std::shared_ptr<AstExpression> &current_value = m_properties.GetIdentifier()->GetCurrentValue();
                ASSERT(current_value != nullptr);

                // set access options for this variable based on those of the current value
                AstExpression::m_access_options = current_value->GetAccessOptions();

                // if alias, accept the current value instead
                current_value->Visit(visitor, mod);
            } else {
                if (m_properties.GetIdentifier()->GetFlags() & IdentifierFlags::FLAG_CONST) {
                    // for const, set access options to only load
                    AstExpression::m_access_options = AccessMode::ACCESS_MODE_LOAD;
                }

                m_properties.GetIdentifier()->IncUseCount();

                if (m_properties.IsInFunction()) {
                    if (m_properties.IsInPureFunction()) {
                        // check if pure function - in a pure function, only variables from this scope may be used
                        if (!mod->LookUpIdentifierDepth(m_name, m_properties.GetDepth())) {
                            // add error that the variable must be passed as a parameter
                            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                                LEVEL_ERROR,
                                Msg_pure_function_scope,
                                m_location,
                                m_name
                            ));
                        }
                    }

                    // NOTE: if we are in a function, and the variable we are loading is declared in a separate function,
                    // we will show an error message saying that the variable must be passed as a parameter to be captured.
                    // the reason for this is that any variables owned by the parent function will be immediately popped from the stack
                    // when the parent function returns. That will mean the variables used here will reference garbage.
                    // In the near feature, it'd be possible to automatically make a copy of those variables referenced and store them
                    // on the stack of /this/ function.
                    if (m_properties.GetIdentifier()->GetFlags() & FLAG_DECLARED_IN_FUNCTION) {
                        // lookup the variable by depth to make sure it was declared in the current function
                        // we do this to make sure it was declared in this scope.
                        if (!mod->LookUpIdentifierDepth(m_name, m_properties.GetDepth())) {
                            Scope *function_scope = m_properties.GetFunctionScope();
                            ASSERT(function_scope != nullptr);

                            function_scope->AddClosureCapture(
                                m_name,
                                m_properties.GetIdentifier()
                            );

                            // closures are objects with a method named '$invoke',
                            // because we are in the '$invoke' method currently,
                            // we use the variable as 'self.<variable name>'
                            m_closure_member_access.reset(new AstMember(
                                m_name,
                                std::shared_ptr<AstVariable>(new AstVariable(
                                    "__closure_self",
                                    m_location
                                )),
                                m_location
                            ));

                            m_closure_member_access->Visit(visitor, mod);
                        }
                    }
                }
            }

            break;
        }
        case IDENTIFIER_TYPE_MODULE:
            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                LEVEL_ERROR,
                Msg_identifier_is_module,
                m_location,
                m_name
            ));
            break;
        case IDENTIFIER_TYPE_TYPE:
            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                LEVEL_ERROR,
                Msg_identifier_is_type,
                m_location,
                m_name
            ));
            break;
        case IDENTIFIER_TYPE_NOT_FOUND:
            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                LEVEL_ERROR,
                Msg_undeclared_identifier,
                m_location,
                m_name,
                mod->GenerateFullModuleName()
            ));
            break;
        default:
            break;
    }
}

std::unique_ptr<Buildable> AstVariable::Build(AstVisitor *visitor, Module *mod)
{
    std::unique_ptr<BytecodeChunk> chunk = BytecodeUtil::Make<BytecodeChunk>();

    if (m_closure_member_access != nullptr) {
        m_closure_member_access->SetAccessMode(m_access_mode);
        return m_closure_member_access->Build(visitor, mod);
    } else {
        ASSERT(m_properties.GetIdentifier() != nullptr);

        // if alias or const, load direct value.
        // if it's an alias then it will just refer to whatever other variable
        // is being referenced. if it is const, load the direct value held in the variable
        const std::shared_ptr<AstExpression> &current_value = m_properties.GetIdentifier()->GetCurrentValue();

        const bool is_alias = m_properties.GetIdentifier()->GetFlags() & IdentifierFlags::FLAG_ALIAS;
        const bool is_const = m_properties.GetIdentifier()->GetFlags() & IdentifierFlags::FLAG_CONST;

        // NOTE: if we are loading a const and current_value == nullptr, proceed with loading the
        // normal way.
        bool should_load_inline = false;

        if (is_alias) {
            ASSERT(current_value != nullptr);
            should_load_inline = true;
        } else if (is_const) {
            if (current_value != nullptr) {
                const SymbolTypePtr_t current_value_type = current_value->GetSymbolType();
                if (current_value_type != nullptr) {
                    // only load basic types inline.
                    if (current_value_type->GetTypeClass() == SymbolTypeClass::TYPE_BUILTIN) {
                        should_load_inline = true;
                    }
                }
            }
        }

        if (should_load_inline) {
            // if alias, accept the current value instead
            const AccessMode current_access_mode = current_value->GetAccessMode();
            current_value->SetAccessMode(m_access_mode);
            chunk->Append(current_value->Build(visitor, mod));
            // reset access mode
            current_value->SetAccessMode(current_access_mode);
        } else {
            int stack_size = visitor->GetCompilationUnit()->GetInstructionStream().GetStackSize();
            int stack_location = m_properties.GetIdentifier()->GetStackLocation();
            int offset = stack_size - stack_location;

            // get active register
            uint8_t rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            if (m_properties.GetIdentifier()->GetFlags() & FLAG_DECLARED_IN_FUNCTION) {
                if (m_access_mode == ACCESS_MODE_LOAD) {
                    // load stack value at offset value into register
                    auto instr_load_offset = BytecodeUtil::Make<StorageOperation>();
                    instr_load_offset->GetBuilder().Load(rp).Local().ByOffset(offset);
                    chunk->Append(std::move(instr_load_offset));
                } else if (m_access_mode == ACCESS_MODE_STORE) {
                    // store the value at (rp - 1) into this local variable
                    auto instr_mov_index = BytecodeUtil::Make<StorageOperation>();
                    instr_mov_index->GetBuilder().Store(rp - 1).Local().ByOffset(offset);
                    chunk->Append(std::move(instr_mov_index));
                }
            } else {
                // load globally, rather than from offset.
                if (m_access_mode == ACCESS_MODE_LOAD) {
                    // load stack value at index into register
                    auto instr_load_index = BytecodeUtil::Make<StorageOperation>();
                    instr_load_index->GetBuilder().Load(rp).Local().ByIndex(stack_location);
                    chunk->Append(std::move(instr_load_index));
                } else if (m_access_mode == ACCESS_MODE_STORE) {
                    // store the value at the index into this local variable
                    auto instr_mov_index = BytecodeUtil::Make<StorageOperation>();
                    instr_mov_index->GetBuilder().Store(rp - 1).Local().ByIndex(stack_location);
                    chunk->Append(std::move(instr_mov_index));
                }
            }
        }
    }

    return std::move(chunk);
}

void AstVariable::Optimize(AstVisitor *visitor, Module *mod)
{
}

Pointer<AstStatement> AstVariable::Clone() const
{
    return CloneImpl();
}

Tribool AstVariable::IsTrue() const
{
    if (m_properties.GetIdentifier() != nullptr) {
        // we can only check if this is true during
        // compile time if it is const literal
        if (m_properties.GetIdentifier()->GetFlags() & FLAG_CONST) {
            if (auto *constant = dynamic_cast<AstConstant*>(m_properties.GetIdentifier()->GetCurrentValue().get())) {
                return constant->IsTrue();
            }
        }
    }

    return Tribool::Indeterminate();
}

bool AstVariable::MayHaveSideEffects() const
{
    // a simple variable reference does not cause side effects
    return false;
}
