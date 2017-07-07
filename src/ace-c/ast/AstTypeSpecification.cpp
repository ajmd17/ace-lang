#include <ace-c/ast/AstTypeSpecification.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Module.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <common/my_assert.hpp>
#include <common/utf8.hpp>

AstTypeSpecification::AstTypeSpecification(
    const std::string &left,
    const std::vector<std::shared_ptr<AstTypeSpecification>> &generic_params,
    const std::shared_ptr<AstTypeSpecification> &right,
    const SourceLocation &location)
    : AstStatement(location),
      m_left(left),
      m_generic_params(generic_params),
      m_right(right),
      m_symbol_type(BuiltinTypes::UNDEFINED),
      m_original_type(BuiltinTypes::UNDEFINED),
      m_is_chained(false)
{
}

void AstTypeSpecification::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(visitor != nullptr && mod != nullptr);

    std::vector<GenericInstanceTypeInfo::Arg> generic_types;

    for (auto &param : m_generic_params) {
        if (param != nullptr) {
            param->Visit(visitor, visitor->GetCompilationUnit()->GetCurrentModule());
            if (param->GetSymbolType()) {
                generic_types.push_back({
                    "of", param->GetSymbolType()
                });
            } else {
                generic_types.push_back({
                    "of", BuiltinTypes::UNDEFINED
                });
            }
        }
    }

    // check user-defined types
    if (m_right == nullptr) {
        if (SymbolTypePtr_t symbol_type = mod->LookupSymbolType(m_left)) {
            m_original_type = symbol_type;

            while (symbol_type != nullptr && symbol_type->GetTypeClass() == TYPE_ALIAS) {
                symbol_type = symbol_type->GetAliasInfo().m_aliasee.lock();
            }

            if (symbol_type->GetTypeClass() == TYPE_GENERIC_PARAMETER) {
                // if it is a generic parameter:
                //   if the substitution has been supplied:
                //     set the type to be that.
                //   else:
                //     set the type to be the param itself.
                if (auto sp = symbol_type->GetGenericParameterInfo().m_substitution.lock()) {
                    // set type to be substituted type
                    m_symbol_type = sp;
                } else {
                    m_symbol_type = symbol_type;
                }
            } else if (symbol_type->GetTypeClass() == TYPE_GENERIC) {
                // check generic params
                if (!m_generic_params.empty()) {
                    // look up generic instance to see if it's already been created
                    if (!(m_symbol_type = visitor->GetCompilationUnit()->
                        GetCurrentModule()->LookupGenericInstance(symbol_type, generic_types))) {
                        // nothing found from lookup,
                        // so create new generic instance
                        if (symbol_type->GetGenericInfo().m_num_parameters == -1 ||
                            symbol_type->GetGenericInfo().m_num_parameters == generic_types.size())
                        {
                            // open the scope for data members
                            mod->m_scopes.Open(Scope());

                            std::vector<SymbolTypePtr_t> substituted_types;
                            substituted_types.reserve(generic_types.size());

                            // for each supplied parameter, create substitution
                            for (size_t i = 0; 
                                i < generic_types.size() && i < symbol_type->GetGenericInfo().m_params.size(); i++)
                            {

                                if (const SymbolTypePtr_t &gen = generic_types[i].m_type) {
                                    SymbolTypePtr_t param_type = SymbolType::GenericParameter(
                                        symbol_type->GetGenericInfo().m_params[i]->GetName(),
                                        gen /* set substitution to the given type */
                                    );

                                    visitor->GetCompilationUnit()->GetCurrentModule()->
                                        m_scopes.Top().GetIdentifierTable().AddSymbolType(param_type);
                                }
                            }

                            SymbolTypePtr_t new_instance = SymbolType::GenericInstance(
                                symbol_type,
                                GenericInstanceTypeInfo {
                                    generic_types
                                }
                            );

                            // accept all members
                            for (auto &mem : new_instance->GetMembers()) {
                                // accept assignment for new member instance
                                if (auto &mem_assignment = std::get<2>(mem)) {
                                    mem_assignment->Visit(visitor, mod);
                                    // update held type to new symbol type
                                    std::get<1>(mem) = mem_assignment->GetSymbolType();
                                } else if (auto &mem_default = std::get<1>(mem)->GetDefaultValue()) {
                                    // assignment is null, update default value
                                    mem_default->Visit(visitor, mod);
                                    //std::get<1>(mem) = mem_default->GetSymbolType();
                                    //std::get<1>(mem) = mem_default->GetSymbolType();
                                }
                            }

                            // close the scope for data members
                            mod->m_scopes.Close();

                            m_symbol_type = new_instance;

                            // add generic instance to be reused
                            visitor->GetCompilationUnit()->GetCurrentModule()->
                                m_scopes.Root().GetIdentifierTable().AddSymbolType(new_instance);
                        } else {
                            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                                LEVEL_ERROR,
                                Msg_generic_parameters_missing,
                                m_location,
                                symbol_type->GetName(),
                                symbol_type->GetGenericInfo().m_num_parameters
                            ));
                        }
                    }
                } else {
                    if (symbol_type->GetDefaultValue() != nullptr) {
                        // if generics have a default value,
                        // allow user to omit parameters
                        m_symbol_type = symbol_type;
                    } else {
                        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                            LEVEL_ERROR,
                            Msg_generic_parameters_missing,
                            m_location,
                            symbol_type->GetName(),
                            symbol_type->GetGenericInfo().m_num_parameters
                        ));
                    }
                }
            } else {
                m_symbol_type = symbol_type;

                if (!m_generic_params.empty()) {
                    // not a generic type but generic params supplied
                    visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                        LEVEL_ERROR,
                        Msg_type_not_generic,
                        m_location,
                        symbol_type->GetName()
                    ));
                }
            }

        } else {
            // error, unknown type
            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                LEVEL_ERROR,
                Msg_undefined_type,
                m_location,
                m_left
            ));
        }



        /* // only the left is here so check this module only
        if (!mod->LookUpUserType(m_left, m_object_type)) {
            // check the global module for the specified type
            if (!visitor->GetCompilationUnit()->GetGlobalModule()->LookUpUserType(m_left, m_object_type)) {
                // first check if there is an identifier with the
                // same name, maybe the user was confused
                if (mod->LookUpIdentifier(m_left, false)) {
                    visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(LEVEL_ERROR, Msg_expected_type_got_identifier, m_location, m_left));
                } else {
                    // error, unknown type
                    visitor->GetCompilationUnit()->GetErrorList().AddError(
                        CompilerError(LEVEL_ERROR, Msg_undefined_type, m_location, m_left));
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
            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                LEVEL_ERROR,
                Msg_unknown_module,
                m_location,
                m_left
            ));
        }
    }
}

std::unique_ptr<Buildable> AstTypeSpecification::Build(AstVisitor *visitor, Module *mod)
{
    return nullptr;
}

void AstTypeSpecification::Optimize(AstVisitor *visitor, Module *mod)
{
}

Pointer<AstStatement> AstTypeSpecification::Clone() const
{
    return CloneImpl();
}
