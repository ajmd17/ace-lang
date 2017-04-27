#include <ace-c/ast/AstMemberAccess.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/StaticObject.hpp>
#include <ace-c/ast/AstVariable.hpp>
#include <ace-c/ast/AstFunctionCall.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>
#include <common/hasher.hpp>

#include <iostream>

AstMemberAccess::AstMemberAccess(const std::shared_ptr<AstExpression> &target,
    const std::vector<std::shared_ptr<AstIdentifier>> &parts,
    const SourceLocation &location)
    : AstExpression(location),
      m_target(target),
      m_parts(parts),
      m_mod_access(nullptr),
      m_access_mode(ACCESS_MODE_LOAD),
      m_side_effects(true)
{
}

void AstMemberAccess::Visit(AstVisitor *visitor, Module *mod)
{
    std::shared_ptr<AstExpression> real_target;
    SymbolTypePtr_t target_type;
    int pos = 0;
    bool has_side_effects = false;

    // check if first item is a module name
    // it should be an instance of AstVariable
    if (AstVariable *first_as_var = dynamic_cast<AstVariable*>(m_target.get())) {
        // check all modules for one with the same name
        m_mod_access = visitor->GetCompilationUnit()->LookupModule(first_as_var->GetName());
    }

    ASSERT(pos < m_parts.size());

    if (m_mod_access) {
        real_target = m_parts[pos++];
    } else {
        real_target = m_target;
    }

    // accept target
    real_target->Visit(visitor, (m_mod_access != nullptr) ? m_mod_access : mod);
    target_type = real_target->GetSymbolType();
    m_part_object_types.push_back(target_type);

    for (; pos < m_parts.size(); pos++) {
        auto &field = m_parts[pos];
        ASSERT(field != nullptr);

        AstFunctionCall *field_as_call = dynamic_cast<AstFunctionCall*>(field.get());

        if (target_type == SymbolType::Builtin::ANY) {
            if (field_as_call) {
                has_side_effects = true;
                // if it's a function, we'll have to check it anyway,
                // just in case it happens to be UCS...
                // lookup the identifier first so we don't add a compiler error on not found
                field_as_call->PerformLookup(visitor, mod);
                if (field_as_call->GetProperties().GetIdentifier()) {
                    field_as_call->Visit(visitor, mod);
                }
            }

            // for the Any type, we will have to load the member from a hash
            // leave target_type as Any
            target_type = SymbolType::Builtin::ANY;
            m_part_object_types.push_back(target_type);
            real_target = field;
        } else {
            // first check if the target has the field
            if (auto member_type = target_type->FindMember(field->GetName())) {
                if (field_as_call) {
                    has_side_effects = true;
                    // visit all args even though this is not a free function
                    // still have to make sure each argument is valid
                    for (auto &arg : field_as_call->GetArguments()) {
                        if (arg) {
                            arg->Visit(visitor, visitor->GetCompilationUnit()->GetCurrentModule());
                        }
                    }

                    auto substituted = SemanticAnalyzer::SubstituteFunctionArgs(
                        visitor,
                        mod,
                        member_type,
                        field_as_call->GetArguments(), 
                        field_as_call->GetLocation()
                    );
                    
                    if (auto as_function_type = substituted.first) {
                        member_type = as_function_type;
                    } else {
                        // member is not a function type
                        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                            Level_fatal,
                            Msg_member_not_a_method, 
                            field->GetLocation(),
                            field_as_call->GetName()
                        ));
                    }
                }
                
                target_type = member_type;
                m_part_object_types.push_back(target_type);
                real_target = field;
            } else {
                // allows functions to be used like they are
                // members (uniform call syntax)
                if (!field_as_call) {
                    // not a function call
                    // error; undefined data member.
                    ASSERT(target_type != nullptr);

                    CompilerError err(Level_fatal, Msg_not_a_data_member, field->GetLocation(),
                        field->GetName(), target_type->GetName());
                    visitor->GetCompilationUnit()->GetErrorList().AddError(err);

                    // set target to Undefined because member was not found
                    target_type = SymbolType::Builtin::UNDEFINED;
                    m_part_object_types.push_back(target_type);
                    real_target = field;

                    break;
                } else {
                    if (field_as_call->MayHaveSideEffects()) {
                        has_side_effects = true;
                    }

                    // in this case it would be usage of uniform call syntax.
                    field->Visit(visitor, mod);
                    target_type = field->GetSymbolType();
                    m_part_object_types.push_back(target_type);
                    real_target = field;
                }
            }
        }
    }

    m_side_effects = has_side_effects;
}

void AstMemberAccess::Build(AstVisitor *visitor, Module *mod)
{
    std::shared_ptr<AstExpression> real_target;
    SymbolTypePtr_t target_type;
    int pos = 0;

    if (m_mod_access) {
        real_target = m_parts[pos++];
    } else {
        real_target = m_target;
    }

    // build target
    real_target->Build(visitor, (m_mod_access != nullptr) ? m_mod_access : mod);
    target_type = real_target->GetSymbolType();
    uint8_t rp;
    // get current register index
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    for (; pos < m_parts.size(); pos++) {
        auto &field = m_parts[pos];
        ASSERT(field != nullptr);

        AstFunctionCall *field_as_call = dynamic_cast<AstFunctionCall*>(field.get());

        std::string field_name = field->GetName();

        if (target_type == SymbolType::Builtin::ANY) {
            // for Any type we will have to load from hash
            uint32_t hash = hash_fnv_1(field_name.c_str());

            if (field_as_call) {
                if (!field_as_call->GetProperties().GetIdentifier()) {
                    // if it is /not/ UCS (because the variable has not been found)
                    // then just optimize it by directly looking for member
                    Compiler::LoadMemberFromHashAndCall(visitor, mod, field_as_call, hash);
                } else {
                    // we have to code in some kind of conditional,
                    // so if the member is not found then perform UCS.

                    int found_member_reg = -1;

                    // the label to jump to the very end
                    StaticObject end_label;
                    end_label.m_type = StaticObject::TYPE_LABEL;
                    end_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

                    // the label to jump to the else-part
                    StaticObject else_label;
                    else_label.m_type = StaticObject::TYPE_LABEL;
                    else_label.m_id = visitor->GetCompilationUnit()->GetInstructionStream().NewStaticId();

                    // build the conditional
                    // get current register index
                    // claim register to hold the object we're loading the member from
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();

                    // compile in the instruction to check if it has the member
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint8_t, uint32_t>(HAS_MEM_HASH, rp, rp - 1, hash);

                    found_member_reg = rp;

                    // store the data in a register
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().IncRegisterUsage();

                    // compare the found member to zero
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t>(CMPZ, found_member_reg);

                    // load the label address from static memory into register 0
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint16_t>(LOAD_STATIC, rp, (uint16_t)else_label.m_id);

                    if (!ace::compiler::Config::use_static_objects) {
                        // fill with padding, for LOAD_ADDR instruction.
                        visitor->GetCompilationUnit()->GetInstructionStream().GetPosition() += 2;
                    }

                    // jump if condition is false or zero.
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t>(JE, rp);

                    // enter the block
                    // the member was found here, so we can call what was in the register.

                    // push args
                    int nargs = field_as_call->GetArguments().size();
                    field_as_call->BuildArgumentsStart(visitor, mod);
                    // invoke it.
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint8_t>(CALL, (uint8_t)found_member_reg, (uint8_t)nargs);

                    // pop args
                    field_as_call->BuildArgumentsEnd(visitor, mod);

                    // unclaim register used to hold the object we're loading the member from
                    visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();

                    // this is the `else` part
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

                    // unclaim for conditional
                    rp = visitor->GetCompilationUnit()->GetInstructionStream().DecRegisterUsage();

                    // set the label's position to where the else-block would be
                    else_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
                    visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(else_label);

                    // member was not found, so use UCS
                    Compiler::BuildUCS(visitor, mod, field_as_call);

                    // set the label's position to after the block,
                    // so we can skip it if the condition is false
                    end_label.m_value.lbl = visitor->GetCompilationUnit()->GetInstructionStream().GetPosition();
                    visitor->GetCompilationUnit()->GetInstructionStream().AddStaticObject(end_label);

                }

            } else {
                // TODO StoreMemberFromHash
                Compiler::LoadMemberFromHash(visitor, mod, hash);
            }

            // get current register index
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            target_type = SymbolType::Builtin::ANY;
            real_target = field;

        } else {
            std::pair<int, SymbolTypePtr_t> dm = { -1, nullptr };// = target_type.GetDataMemberIndex(field_name);

            for (size_t i = 0; i < target_type->GetMembers().size(); i++) {
                if (std::get<0>(target_type->GetMembers()[i]) == field_name) {
                    dm = { i, std::get<1>(target_type->GetMembers()[i]) };
                    break;
                }
            }

            if (dm.first != -1) {
                if (field_as_call) {
                    Compiler::LoadMemberAtIndexAndCall(visitor, mod, field_as_call, dm.first);
                } else {
                    if (m_access_mode == ACCESS_MODE_LOAD || pos != m_parts.size() - 1) {
                        // just load the data member.
                        Compiler::LoadMemberAtIndex(visitor, mod, dm.first);
                    } else if (m_access_mode == ACCESS_MODE_STORE) {
                        // we are in storing mode, so store to LAST item in the member expr.
                        Compiler::StoreMemberAtIndex(visitor, mod, dm.first);
                    }
                }

                // get current register index
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                target_type = dm.second;
                real_target = field;
            } else if (field_as_call) {
                Compiler::BuildUCS(visitor, mod, field_as_call);

                // get current register index
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

                target_type = field->GetSymbolType();
                real_target = field;
            } else {
                break;
            }
        }
    }
}

void AstMemberAccess::Optimize(AstVisitor *visitor, Module *mod)
{
    std::shared_ptr<AstExpression> real_target;
    SymbolTypePtr_t target_type;
    int pos = 0;

    ASSERT(pos < m_parts.size());

    if (m_mod_access != nullptr) {
        real_target = m_parts[pos++];
    } else {
        real_target = m_target;
    }

    // accept target
    real_target->Optimize(visitor, (m_mod_access != nullptr) ? m_mod_access : mod);
    target_type = real_target->GetSymbolType();

    for (; pos < m_parts.size(); pos++) {
        auto &field = m_parts[pos];
        ASSERT(field != nullptr);

        AstFunctionCall *field_as_call = dynamic_cast<AstFunctionCall*>(field.get());

        // first check if the target has the field
        if (auto member_type = target_type->FindMember(field->GetName())) {
            if (field_as_call != nullptr) {
                // optimize all args
                for (auto &arg : field_as_call->GetArguments()) {
                    if (arg != nullptr) {
                        arg->Optimize(visitor,
                            visitor->GetCompilationUnit()->GetCurrentModule());
                    }
                }
            }

            target_type = member_type;
            real_target = field;
        } else {
            if (!field_as_call) {
                break;
            } else {
                // in this case it would be usage of uniform call syntax
                field->Optimize(visitor, mod);
                target_type = field->GetSymbolType();
                real_target = field;
            }
        }
    }
}

void AstMemberAccess::Recreate(std::ostringstream &ss)
{
    ASSERT(m_target != nullptr);

    m_target->Recreate(ss);
    ss << ".";

    for (auto &part : m_parts) {
        if (part) {
            part->Recreate(ss);
            ss << ".";
        }
    }
}

Pointer<AstStatement> AstMemberAccess::Clone() const
{
    return CloneImpl();
}

int AstMemberAccess::IsTrue() const
{
    return -1;
}

bool AstMemberAccess::MayHaveSideEffects() const
{
    return m_side_effects;
}

SymbolTypePtr_t AstMemberAccess::GetSymbolType() const
{
    return (!m_part_object_types.empty())
        ? m_part_object_types.back()
        : SymbolType::Builtin::UNDEFINED;
}
