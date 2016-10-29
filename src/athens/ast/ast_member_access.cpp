#include <athens/ast/ast_member_access.hpp>
#include <athens/emit/instruction.hpp>
#include <athens/emit/static_object.hpp>
#include <athens/ast/ast_variable.hpp>
#include <athens/ast/ast_function_call.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/module.hpp>

#include <common/instructions.hpp>

#include <cassert>
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
    ObjectType target_type;
    int pos = 0;
    bool has_side_effects = false;

    // check if first item is a module name
    // it should be an instance of AstVariable
    AstVariable *first_as_var = dynamic_cast<AstVariable*>(m_target.get());
    if (first_as_var != nullptr) {
        // check all modules for one with the same name
        for (int i = 0; i < visitor->GetCompilationUnit()->m_modules.size(); i++) {
            auto &current = visitor->GetCompilationUnit()->m_modules[i];
            if (current != nullptr && current->GetName() == first_as_var->GetName()) {
                // module with name found
                m_mod_access = current.get();
                break;
            }
        }
    }

    assert(pos < m_parts.size());

    if (m_mod_access != nullptr) {
        real_target = m_parts[pos++];
    } else {
        real_target = m_target;
    }

    // accept target
    real_target->Visit(visitor, (m_mod_access != nullptr) ? m_mod_access : mod);
    target_type = real_target->GetObjectType();
    m_part_object_types.push_back(target_type);

    for (; pos < m_parts.size(); pos++) {
        auto &field = m_parts[pos];
        assert(field != nullptr);

        AstFunctionCall *field_as_call = dynamic_cast<AstFunctionCall*>(field.get());

        // first check if the target has the field
        if (target_type.HasDataMember(field->GetName())) {
            if (field_as_call != nullptr) {
                has_side_effects = true;
                // visit all args even though this is not a free function
                // still have to make sure each argument is valid
                for (auto &arg : field_as_call->GetArguments()) {
                    if (arg != nullptr) {
                        arg->Visit(visitor,
                            visitor->GetCompilationUnit()->GetCurrentModule().get());
                    }
                }
            }

            target_type = target_type.GetDataMemberType(field->GetName());
            m_part_object_types.push_back(target_type);
            real_target = field;
        } else {
            // allows functions to be used like they are
            // members (uniform call syntax)
            if (field_as_call == nullptr) {
                if (field_as_call->MayHaveSideEffects()) {
                    has_side_effects = true;
                }
                // error; undefined data member.
                CompilerError err(Level_fatal, Msg_not_a_data_member, field->GetLocation(),
                    field->GetName(), target_type.ToString());
                visitor->GetCompilationUnit()->GetErrorList().AddError(err);
                break;
            } else {
                // in this case it would be usage of uniform call syntax.
                field->Visit(visitor, mod);
                target_type = field->GetObjectType();
                m_part_object_types.push_back(target_type);
                real_target = field;
            }
        }
    }

    m_side_effects = has_side_effects;
}

void AstMemberAccess::Build(AstVisitor *visitor, Module *mod)
{
    std::shared_ptr<AstExpression> real_target;
    ObjectType target_type;
    int pos = 0;

    if (m_mod_access != nullptr) {
        real_target = m_parts[pos++];
    } else {
        real_target = m_target;
    }

    // build target
    real_target->Build(visitor, (m_mod_access != nullptr) ? m_mod_access : mod);
    target_type = real_target->GetObjectType();
    uint8_t rp;
    // get current register index
    rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

    for (; pos < m_parts.size(); pos++) {
        auto &field = m_parts[pos];
        assert(field != nullptr);

        AstFunctionCall *field_as_call = dynamic_cast<AstFunctionCall*>(field.get());

        int dm_index = target_type.GetDataMemberIndex(field->GetName());
        if (dm_index != -1) {
            if (field_as_call != nullptr) {
                // push args
                int nargs = field_as_call->GetArguments().size();
                field_as_call->BuildArgumentsStart(visitor, mod);
                // get current register index
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                // load data member.
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(LOAD_MEM, rp, rp, (uint8_t)dm_index);
                // invoke it.
                visitor->GetCompilationUnit()->GetInstructionStream() <<
                    Instruction<uint8_t, uint8_t, uint8_t>(CALL, rp, (uint8_t)nargs);
                // pop args
                field_as_call->BuildArgumentsEnd(visitor, mod);
                // get current register index
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
            } else {
                // get current register index
                rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();
                if (m_access_mode == ACCESS_MODE_LOAD || pos != m_parts.size() - 1) {
                    // just load the data member.
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(LOAD_MEM, rp, rp, (uint8_t)dm_index);
                } else if (m_access_mode == ACCESS_MODE_STORE) {
                    // we are in storing mode, so store the LAST item in the member expr.
                    visitor->GetCompilationUnit()->GetInstructionStream() <<
                        Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(MOV_MEM, rp, (uint8_t)dm_index, rp - 1);
                }
            }

            target_type = target_type.GetDataMemberType(field->GetName());
            real_target = field;
        } else if (field_as_call != nullptr) {
            // allows functions to be used like they are
            // members (uniform call syntax)
            // in this case it would be usage of uniform call syntax.

            // build what we have so far into the function
            // get active register
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

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

            // get current register index
            rp = visitor->GetCompilationUnit()->GetInstructionStream().GetCurrentRegister();

            target_type = field->GetObjectType();
            real_target = field;
        } else {
            break;
        }
    }
}

void AstMemberAccess::Optimize(AstVisitor *visitor, Module *mod)
{
    std::shared_ptr<AstExpression> real_target;
    ObjectType target_type;
    int pos = 0;

    assert(pos < m_parts.size());

    if (m_mod_access != nullptr) {
        real_target = m_parts[pos++];
    } else {
        real_target = m_target;
    }

    // accept target
    real_target->Optimize(visitor, (m_mod_access != nullptr) ? m_mod_access : mod);
    target_type = real_target->GetObjectType();

    for (; pos < m_parts.size(); pos++) {
        auto &field = m_parts[pos];
        assert(field != nullptr);

        AstFunctionCall *field_as_call = dynamic_cast<AstFunctionCall*>(field.get());

        // first check if the target has the field
        if (target_type.HasDataMember(field->GetName())) {
            if (field_as_call != nullptr) {
                // optimize all args
                for (auto &arg : field_as_call->GetArguments()) {
                    if (arg != nullptr) {
                        arg->Optimize(visitor,
                            visitor->GetCompilationUnit()->GetCurrentModule().get());
                    }
                }
            }

            target_type = target_type.GetDataMemberType(field->GetName());
            real_target = field;
        } else {
            if (field_as_call == nullptr) {
                break;
            } else {
                // in this case it would be usage of uniform call syntax.
                field->Optimize(visitor, mod);
                target_type = field->GetObjectType();
                real_target = field;
            }
        }
    }
}

int AstMemberAccess::IsTrue() const
{
    return -1;
}

bool AstMemberAccess::MayHaveSideEffects() const
{
    return m_side_effects;
}

ObjectType AstMemberAccess::GetObjectType() const
{
    return (!m_part_object_types.empty())
        ? m_part_object_types.back()
        : ObjectType::type_builtin_undefined;
}
