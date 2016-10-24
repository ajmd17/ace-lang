#include <athens/ast/ast_member_access.hpp>
#include <athens/emit/instruction.hpp>
#include <athens/emit/static_object.hpp>
#include <athens/ast/ast_function_call.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/module.hpp>

#include <common/instructions.hpp>

AstMemberAccess::AstMemberAccess(std::shared_ptr<AstExpression> left,
    std::shared_ptr<AstExpression> right,
    const SourceLocation &location)
    : AstExpression(location),
      m_left(left),
      m_right(right),
      m_lhs_mod(nullptr)
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
        // TODO: object member access
        m_left->Visit(visitor, mod);
        // accept the right-hand side
        m_right->Visit(visitor, mod);
    }
}

void AstMemberAccess::Build(AstVisitor *visitor, Module *mod)
{
    if (m_lhs_mod != nullptr) {
        // simple module access such as SomeModule.dosomething()

        // module index has already been set while analyzing
        mod = m_lhs_mod;
    } else {
        // check to see if it is a function call on an object
        // such as: myobject.dosomething()

        AstExpression *rightmost = m_right.get();
        AstMemberAccess *rightmost_as_mem = dynamic_cast<AstMemberAccess*>(rightmost);
        while (rightmost_as_mem != nullptr) {
            rightmost = rightmost_as_mem->GetRight().get();
            rightmost_as_mem = dynamic_cast<AstMemberAccess*>(rightmost);
        }

        // check if right-hand side is a function call
        AstFunctionCall *rightmost_as_function_call = dynamic_cast<AstFunctionCall*>(rightmost);
        if (rightmost_as_function_call != nullptr) {
            // it is function call on an object
            // add the left hand side to the parameters
            rightmost_as_function_call->AddArgument(m_left);
        } else {
            // TODO: object member access
        }
    }

    // accept the next part of this member access
    m_right->Build(visitor, mod);
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
