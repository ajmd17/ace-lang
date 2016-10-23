#include <athens/ast/ast_member_access.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/emit/instruction.hpp>
#include <athens/emit/static_object.hpp>

#include <common/instructions.hpp>

AstMemberAccess::AstMemberAccess(std::shared_ptr<AstExpression> left,
    std::shared_ptr<AstExpression> right,
    const SourceLocation &location)
    : AstExpression(location),
      m_left(left),
      m_right(right),
      m_lhs_mod(false),
      m_mod_index(0),
      m_mod_index_before(0)
{
}

void AstMemberAccess::Visit(AstVisitor *visitor)
{
    // store the module index
    m_mod_index = visitor->GetCompilationUnit()->m_module_index;
    m_mod_index_before = m_mod_index;

    AstIdentifier *left_as_identifier = dynamic_cast<AstIdentifier*>(m_left.get());
    if (left_as_identifier != nullptr) {
        // check if left-hand side is a module
        for (int i = 0; i < visitor->GetCompilationUnit()->m_modules.size(); i++) {
            auto &mod = visitor->GetCompilationUnit()->m_modules[i];
            if (mod != nullptr && mod->GetName() == left_as_identifier->GetName()) {
                m_lhs_mod = true;

                // module with name found, change module index
                m_mod_index = i;
                visitor->GetCompilationUnit()->m_module_index = m_mod_index;

                break;
            }
        }
    }

    if (!m_lhs_mod) {
        // TODO: object member access
        m_left->Visit(visitor);
    }

    // accept the right-hand side
    m_right->Visit(visitor);

    // reset the module index
    visitor->GetCompilationUnit()->m_module_index = m_mod_index_before;
}

void AstMemberAccess::Build(AstVisitor *visitor)
{
    // set new module index
    visitor->GetCompilationUnit()->m_module_index = m_mod_index;

    if (!m_lhs_mod) {
        // TODO: object member access
        m_left->Build(visitor);
    }

    // accept the right-hand side
    m_right->Build(visitor);

    // reset the module index
    visitor->GetCompilationUnit()->m_module_index = m_mod_index_before;
}

void AstMemberAccess::Optimize(AstVisitor *visitor)
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
