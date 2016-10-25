#include <athens/ast/ast_type_specification.hpp>
#include <athens/ast_visitor.hpp>
#include <athens/module.hpp>

AstTypeSpecification::AstTypeSpecification(const std::string &left,
    const std::shared_ptr<AstTypeSpecification> &right,
    const SourceLocation &location)
    : AstStatement(location),
      m_left(left),
      m_right(right)
{
}

void AstTypeSpecification::Visit(AstVisitor *visitor, Module *mod)
{
    // first, check builtin types
    const ObjectType *type_builtin = ObjectType::GetBuiltinType(m_left);
    if (type_builtin != nullptr) {
        m_object_type = *type_builtin;
    } else {
        // now, check user-defined types
        if (m_right == nullptr) {
            // only the left is here so check this module only
            if (!mod->LookUpUserType(m_left, m_object_type)) {
                // first check if there is an identifier with the
                // same name, maybe the user was confused
                if (mod->LookUpIdentifier(m_left, false)) {
                    visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(Level_fatal, Msg_expected_type_got_identifier, m_location, m_left));
                } else {
                    // error, unknown type
                    visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(Level_fatal, Msg_undefined_type, m_location, m_left));
                }
            }
        } else {
            // module access type
            Module *left_mod = nullptr;

            // check if left is a module name
            for (int i = 0; i < visitor->GetCompilationUnit()->m_modules.size(); i++) {
                auto &found_mod = visitor->GetCompilationUnit()->m_modules[i];
                if (found_mod != nullptr && found_mod->GetName() == m_left) {

                    // module with name found
                    left_mod = found_mod.get();
                    // accept the right-hand side
                    m_right->Visit(visitor, left_mod);

                    // set m_object_type to be the right hand's calculated one
                    m_object_type = m_right->GetObjectType();

                    return;
                }
            }

            if (left_mod == nullptr) {
                // did not find module
                visitor->GetCompilationUnit()->GetErrorList().AddError(
                    CompilerError(Level_fatal, Msg_unknown_module, m_location, m_left));
            }
        }
    }
}

void AstTypeSpecification::Build(AstVisitor *visitor, Module *mod)
{
}

void AstTypeSpecification::Optimize(AstVisitor *visitor, Module *mod)
{
}
