#include <ace-c/ast/ast_type_specification.hpp>
#include <ace-c/ast_visitor.hpp>
#include <ace-c/module.hpp>

#include <common/my_assert.hpp>

AstTypeSpecification::AstTypeSpecification(const std::string &left,
    const std::shared_ptr<AstTypeSpecification> &right,
    const SourceLocation &location)
    : AstStatement(location),
      m_left(left),
      m_right(right),
      m_is_chained(false)
{
}

void AstTypeSpecification::Visit(AstVisitor *visitor, Module *mod)
{
    // first, check builtin types
    if (const ObjectType *type_builtin = ObjectType::GetBuiltinType(m_left)) {
        m_object_type = *type_builtin;
    } else {
        // now, check user-defined types
        if (!m_right) {
            // only the left is here so check this module only
            if (!mod->LookUpUserType(m_left, m_object_type)) {
                // check the global module for the specified type
                if (!visitor->GetCompilationUnit()->GetGlobalModule()->LookUpUserType(m_left, m_object_type)) {
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
            }
        } else {
            Module *left_mod = nullptr;

            if (m_is_chained) {
                ASSERT(mod != nullptr);
                ASSERT(mod->GetImportTreeLink() != nullptr);

                // search siblings of the current module,
                // rather than global lookup.
                for (auto *sibling : mod->GetImportTreeLink()->m_siblings) {
                    ASSERT(sibling != nullptr);
                    ASSERT(sibling->m_value != nullptr);

                    if (sibling->m_value->GetName() == m_left) {
                        left_mod = sibling->m_value;
                    }
                }
            } else {
                left_mod = visitor->GetCompilationUnit()->LookupModule(m_left);
            }

            if (m_right->m_right) {
                // set next to chained
                m_right->m_is_chained = true;
            }

            // module access type
            if (left_mod) {
                // accept the right-hand side
                m_right->Visit(visitor, left_mod);
                // set m_object_type to be the right hand's calculated one
                m_object_type = m_right->GetObjectType();
            } else {
                // did not find module
                CompilerError err(Level_fatal, Msg_unknown_module, m_location, m_left);
                visitor->GetCompilationUnit()->GetErrorList().AddError(err);
                m_object_type = ObjectType::type_builtin_undefined;
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
