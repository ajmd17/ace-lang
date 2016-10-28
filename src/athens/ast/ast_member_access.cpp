#include <athens/ast/ast_member_access.hpp>
#include <athens/emit/instruction.hpp>
#include <athens/emit/static_object.hpp>
#include <athens/ast/ast_variable.hpp>
#include <athens/ast/ast_function_call.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/module.hpp>

#include <common/instructions.hpp>

#include <cassert>

AstMemberAccess::AstMemberAccess(const std::shared_ptr<AstExpression> &target,
    const std::vector<std::shared_ptr<AstIdentifier>> &parts,
    const SourceLocation &location)
    : AstExpression(location),
      m_target(target),
      m_parts(parts),
      m_mod_access(nullptr)
{
}

void AstMemberAccess::Visit(AstVisitor *visitor, Module *mod)
{
    std::shared_ptr<AstExpression> real_target;
    int pos = 0;

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

    for (; pos < m_parts.size(); pos++) {
        auto &field = m_parts[pos];
        assert(field != nullptr);

        // first check if the target's has the field
        ObjectType target_type = real_target->GetObjectType();

        if (target_type.HasDataMember(field->GetName())) {
            real_target = field;
        } else {
            // allows functions to be used like they are
            // members (uniform call syntax)
            if (dynamic_cast<AstFunctionCall*>(field.get()) == nullptr) {
                // error; undefined data member.
                CompilerError err(Level_fatal, Msg_not_a_data_member, field->GetLocation(),
                    field->GetName(), target_type.ToString());
                visitor->GetCompilationUnit()->GetErrorList().AddError(err);
                break;
            } else {
                // in this case it would be usage of uniform call syntax.
                field->Visit(visitor, mod);
                real_target = field;
            }
        }
    }

}

void AstMemberAccess::Build(AstVisitor *visitor, Module *mod)
{

}

void AstMemberAccess::Optimize(AstVisitor *visitor, Module *mod)
{
}

int AstMemberAccess::IsTrue() const
{
    // TODO
    return -1;
}

bool AstMemberAccess::MayHaveSideEffects() const
{
    // TODO
    return true;
}

ObjectType AstMemberAccess::GetObjectType() const
{
    return (m_parts.back() != nullptr)
        ? m_parts.back()->GetObjectType()
        : ObjectType::type_builtin_undefined;
}
