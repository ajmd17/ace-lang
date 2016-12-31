#include <ace-c/ast/AstTypeSpecification.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>

#include <common/my_assert.hpp>
#include <common/utf8.hpp>

AstTypeSpecification::AstTypeSpecification(const std::string &left,
    const std::vector<std::shared_ptr<AstTypeSpecification>> &generic_params,
    const std::shared_ptr<AstTypeSpecification> &right,
    const SourceLocation &location)
    : AstStatement(location),
      m_left(left),
      m_generic_params(generic_params),
      m_right(right),
      m_symbol_type(SymbolType::Builtin::UNDEFINED),
      m_is_chained(false)
{
}

void AstTypeSpecification::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr && mod != nullptr);

    std::vector<SymbolTypePtr_t> generic_types;

    for (auto &param : m_generic_params) {
        if (param) {
            param->Visit(visitor, visitor->GetCompilationUnit()->GetCurrentModule());
            if (param->GetSymbolType()) {
                generic_types.push_back(param->GetSymbolType());
            }
        }
    }

    // check user-defined types
    if (!m_right) {
        if (auto symbol_type = mod->LookupSymbolType(m_left)) {
            // check generic params
            if (!m_generic_params.empty()) {
                if (symbol_type->GetTypeClass() == TYPE_GENERIC) {
                    // create generic instance
                    if (symbol_type->GetGenericInfo().m_num_parameters == -1 || symbol_type->GetGenericInfo().m_num_parameters == generic_types.size()) {
                        m_symbol_type = SymbolType::GenericInstance(symbol_type, GenericInstanceTypeInfo{ generic_types });
                    } else {
                        visitor->GetCompilationUnit()->GetErrorList().AddError(
                            CompilerError(Level_fatal, Msg_generic_parameters_missing, m_location,
                                symbol_type->GetName(), symbol_type->GetGenericInfo().m_num_parameters));
                    }
                } else {
                    // not a generic type
                    visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(Level_fatal, Msg_type_not_generic, m_location, symbol_type->GetName()));
                }
            } else {
                if (symbol_type->GetTypeClass() == TYPE_GENERIC) {
                    m_symbol_type = symbol_type;
                    /*visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(Level_fatal, Msg_generic_parameters_missing, m_location,
                            symbol_type->GetName(), symbol_type->GetGenericInfo().m_num_parameters));*/
                } else {
                    m_symbol_type = symbol_type;
                }
            }

        } else {
            // error, unknown type
            visitor->GetCompilationUnit()->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_undefined_type, m_location, m_left));
        }



        /* // only the left is here so check this module only
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
        }*/
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

            m_symbol_type = m_right->GetSymbolType();
        } else {
            // did not find module
            CompilerError err(Level_fatal, Msg_unknown_module, m_location, m_left);
            visitor->GetCompilationUnit()->GetErrorList().AddError(err);
        }
    }
}

void AstTypeSpecification::Build(AstVisitor *visitor, Module *mod)
{
}

void AstTypeSpecification::Optimize(AstVisitor *visitor, Module *mod)
{
}

void AstTypeSpecification::Recreate(std::ostringstream &ss)
{
    ss << m_left;
    if (m_right) {
        ss << "::";
        m_right->Recreate(ss);
    }
}