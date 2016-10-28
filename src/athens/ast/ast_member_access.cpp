#include <athens/ast/ast_member_access.hpp>
#include <athens/emit/instruction.hpp>
#include <athens/emit/static_object.hpp>
#include <athens/ast/ast_function_call.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/module.hpp>

#include <common/instructions.hpp>

#include <cassert>

AstMemberAccess::AstMemberAccess(std::shared_ptr<AstExpression> left,
    std::shared_ptr<AstExpression> right,
    const SourceLocation &location)
    : AstExpression(location),
      m_left(left),
      m_right(right),
      m_lhs_mod(nullptr),
      m_is_free_call(false),
      m_is_mem_chain(false)
{
}

void AstMemberAccess::Visit(AstVisitor *visitor, Module *mod)
{
    AstIdentifier *left_as_identifier = dynamic_cast<AstIdentifier*>(m_left.get());
    if (left_as_identifier != nullptr) {
        // check if left-hand side is a module
        for (int i = 0; i < visitor->GetCompilationUnit()->m_modules.size(); i++) {
            auto &found_mod = visitor->GetCompilationUnit()->m_modules[i];
            if (found_mod != nullptr && found_mod->GetName() == left_as_identifier->GetName()) {

                // module with name found, change module index
                m_lhs_mod = found_mod.get();
                // accept the right-hand side
                m_right->Visit(visitor, m_lhs_mod);

                return;
            }
        }
    }

    if (m_lhs_mod == nullptr) {
        // first check for object member access
        m_left->Visit(visitor, mod);

        ObjectType left_type = m_left->GetObjectType();

        AstIdentifier *right_as_identifier = dynamic_cast<AstIdentifier*>(m_right.get());
        // try to cast right as member access as well,
        // and set right_as_identifier to be the left-hand side of the next member access
        if (right_as_identifier == nullptr) {
            AstMemberAccess *right_as_mem = dynamic_cast<AstMemberAccess*>(m_right.get());
            if (right_as_mem != nullptr) {
                m_is_mem_chain = true;
                right_as_identifier = dynamic_cast<AstIdentifier*>(right_as_mem->GetLeft().get());
            }
        }

        if (right_as_identifier != nullptr) {
            if (!left_type.HasDataMember(right_as_identifier->GetName())) {
                AstFunctionCall *right_as_function_call = dynamic_cast<AstFunctionCall*>(right_as_identifier);
                if (right_as_function_call != nullptr) {
                    // accept the right-hand side, for uniform call syntax
                    m_right->Visit(visitor, mod);
                    // free function call
                    m_is_free_call = true;
                } else {
                    // error; data member undefined.
                    visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(Level_fatal, Msg_not_a_data_member, m_right->GetLocation(),
                            right_as_identifier->GetName(), left_type.ToString()));
                }
            }
        } else {
            // error; right side must be an identifier.
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_expected_identifier, m_right->GetLocation()));
        }
    }
}

void AstMemberAccess::Build(AstVisitor *visitor, Module *mod)
{
    if (m_lhs_mod != nullptr) {
        // simple module access
        m_right->Build(visitor, m_lhs_mod);
    } else {
        if (m_is_free_call) {
            // free function call on an object
            // such as: myobject.dosomething()
            AstFunctionCall *right_as_function_call = dynamic_cast<AstFunctionCall*>(m_right.get());
            assert(right_as_function_call != nullptr && "There was an error in analysis");
            right_as_function_call->AddArgumentToFront(m_left);
            m_right->Build(visitor, mod);
        } else if (m_is_mem_chain) {
            m_right->Build(visitor, mod);
        } else {
            // object member access TODO
            assert(0 && "member access not implemented yet");
        }
    }
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
    const AstExpression *rightmost = m_right.get();
    const AstMemberAccess *rightmost_as_mem = dynamic_cast<const AstMemberAccess*>(rightmost);
    while (rightmost_as_mem != nullptr) {
        rightmost = rightmost_as_mem->GetRight().get();
        rightmost_as_mem = dynamic_cast<const AstMemberAccess*>(rightmost);
    }
    return rightmost->GetObjectType();
}
