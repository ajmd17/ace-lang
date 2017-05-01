#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/ast/AstModuleDeclaration.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/SymbolType.hpp>

#include <common/my_assert.hpp>

#include <iostream>

IdentifierLookupResult SemanticAnalyzer::LookupIdentifier(AstVisitor *visitor, Module *mod, const std::string &name)
{
    IdentifierLookupResult res;
    res.type = IDENTIFIER_TYPE_UNKNOWN;

    // the variable must exist in the active scope or a parent scope
    if ((res.as_identifier = mod->LookUpIdentifier(name, false))) {
        res.type = IDENTIFIER_TYPE_VARIABLE;
    } else if ((res.as_identifier = visitor->GetCompilationUnit()->GetGlobalModule()->LookUpIdentifier(name, false))) {
        // if the identifier was not found,
        // look in the global module to see if it is a global function.
        res.type = IDENTIFIER_TYPE_VARIABLE;
    } else if ((res.as_module = visitor->GetCompilationUnit()->LookupModule(name))) {
        res.type = IDENTIFIER_TYPE_MODULE;
    } else if ((res.as_type = mod->LookupSymbolType(name))) {
        res.type = IDENTIFIER_TYPE_TYPE;
    } else {
        // nothing was found
        res.type = IDENTIFIER_TYPE_NOT_FOUND;
    }
    
    return res;
}

void CheckArgTypeCompatible(
    AstVisitor *visitor,
    const SourceLocation &location,
    const SymbolTypePtr_t &arg_type,
    const SymbolTypePtr_t &param_type)
{
    ASSERT(arg_type != nullptr);
    ASSERT(param_type != nullptr);

    // make sure argument types are compatible
    // use strict numbers so that floats cannot be passed as explicit ints
    if (!param_type->TypeCompatible(*arg_type, true)) {
        const CompilerError err(
            Level_fatal,
            Msg_arg_type_incompatible,
            location,
            arg_type->GetName(),
            param_type->GetName()
        );

        visitor->GetCompilationUnit()->GetErrorList().AddError(err);
    }
}

std::pair<SymbolTypePtr_t, std::vector<int>> SemanticAnalyzer::SubstituteFunctionArgs(
    AstVisitor *visitor, Module *mod, 
    const SymbolTypePtr_t &identifier_type, 
    const std::vector<std::shared_ptr<AstArgument>> &args,
    const SourceLocation &location)
{
    if (identifier_type->GetTypeClass() == TYPE_GENERIC_INSTANCE) {
        auto base = identifier_type->GetBaseType();

        if (base == SymbolType::Builtin::FUNCTION) {
            // the indices of the arguments (will be returned)
            std::vector<int> substituted_param_ids;

            auto &generic_args = identifier_type->GetGenericInstanceInfo().m_generic_args;

            // check for varargs (at end)
            bool is_varargs = false;
            SymbolTypePtr_t vararg_type;

            if (!generic_args.empty() && generic_args.back().second->GetTypeClass() == TYPE_GENERIC_INSTANCE) {
                auto base = generic_args.back().second->GetBaseType();
                if (base == SymbolType::Builtin::VAR_ARGS) {
                    is_varargs = true;

                    ASSERT(!generic_args.back().second->GetGenericInstanceInfo().m_generic_args.empty());
                    vararg_type = generic_args.back().second->GetGenericInstanceInfo().m_generic_args[0].second;
                }
            }

            if (generic_args.size() == args.size() + 1 || (is_varargs && generic_args.size() - 1 < args.size() + 2)) {
                
                for (int i = 0; i < args.size(); i++) {
                    ASSERT(args[i] != nullptr);

                    struct {
                        bool is_named;
                        std::string name;
                        SymbolTypePtr_t type;
                    } arg_info;

                    arg_info.is_named = args[i]->IsNamed();
                    arg_info.name = args[i]->GetName();
                    arg_info.type = args[i]->GetSymbolType();

                    // if it is named, search the params for one with the same name.
                    // (make sure it isnt already substituted)
                    if (arg_info.is_named) {
                        // TODO: maybe named varargs could be put in a hashmap?
                        int found_index = -1;

                        for (int j = 1; j < generic_args.size(); j++) {
                            if (generic_args[j].first == arg_info.name) {
                                found_index = j - 1; // do -1 because first param will be return type
                                // found, stop searching
                                break;
                            }
                        }

                        if (found_index == -1) {
                            // still -1, so add error
                            const CompilerError err(
                                Level_fatal,
                                Msg_named_arg_not_found,
                                args[i]->GetLocation(),
                                arg_info.name
                            );

                            visitor->GetCompilationUnit()->GetErrorList().AddError(err);
                        } else {
                            // found successfully, check type compatibility

                            const SymbolTypePtr_t &param_type = generic_args[found_index + 1].second;

                            CheckArgTypeCompatible(
                                visitor, args[i]->GetLocation(), arg_info.type, param_type
                            );
                        }

                        // add 'i' to param ids (index of named param)
                        substituted_param_ids.push_back(found_index);


                    } else {
                        // choose whether to get next param, or last (in the case of varargs)
                        struct {
                            std::string name;
                            SymbolTypePtr_t type;    
                        } param_info;

                        const std::pair<std::string, SymbolTypePtr_t> &param =
                            (i + 1 < generic_args.size())
                            ? generic_args[i + 1]
                            : generic_args.back();
                        
                        param_info.name = param.first;
                        param_info.type = param.second;

                        if (is_varargs && i + 2 >= generic_args.size()) {
                            // in varargs... check vararg base type
                            CheckArgTypeCompatible(
                                visitor, args[i]->GetLocation(), arg_info.type, vararg_type
                            );
                        } else {
                            CheckArgTypeCompatible(
                                visitor, args[i]->GetLocation(), arg_info.type, param_info.type
                            );
                        }

                        // add 'i' to param ids (positional)
                        substituted_param_ids.push_back(i);
                    }
                }
            } else {
                // wrong number of args given
                ErrorMessage msg = (args.size() + 1 > identifier_type->GetGenericInstanceInfo().m_generic_args.size())
                    ? Msg_too_many_args : Msg_too_few_args;
                
                visitor->GetCompilationUnit()->GetErrorList().AddError(
                    CompilerError(Level_fatal, msg, location)
                );
            }

            // make sure the "return type" of the function is not null
            ASSERT(identifier_type->GetGenericInstanceInfo().m_generic_args.size() >= 1);
            ASSERT(identifier_type->GetGenericInstanceInfo().m_generic_args[0].second != nullptr);

            // return the "return type" of the function
            return {
                identifier_type->GetGenericInstanceInfo().m_generic_args[0].second,
                substituted_param_ids
            };
        }
    } else if (identifier_type == SymbolType::Builtin::FUNCTION ||
               identifier_type == SymbolType::Builtin::ANY) {
        // the indices of the arguments (will be returned)
        std::vector<int> substituted_param_ids;

        for (size_t i = 0; i < args.size(); i++) {
            substituted_param_ids.push_back(i);
        }

        // abstract function, allow any params
        return {
            SymbolType::Builtin::ANY,
            substituted_param_ids
        };
    }
    
    return { nullptr, {} };
}

SemanticAnalyzer::SemanticAnalyzer(AstIterator *ast_iterator, CompilationUnit *compilation_unit)
    : AstVisitor(ast_iterator, compilation_unit)
{
}

SemanticAnalyzer::SemanticAnalyzer(const SemanticAnalyzer &other)
    : AstVisitor(other.m_ast_iterator, other.m_compilation_unit)
{
}

void SemanticAnalyzer::Analyze(bool expect_module_decl)
{
    if (expect_module_decl) {
        while (m_ast_iterator->HasNext()) {
            auto first_statement = m_ast_iterator->Next();
            auto module_declaration = std::dynamic_pointer_cast<AstModuleDeclaration>(first_statement);

            if (module_declaration) {
                module_declaration->Visit(this, nullptr);

               /* if (module_declaration->GetModule()) {
                    // module was successfully added

                    // parse filename
                    std::vector<std::string> path = str_util::split_path(module_declaration->GetLocation().GetFileName());
                    path = str_util::canonicalize_path(path);
                    // change it back to string
                    std::string canon_path = str_util::path_to_str(path);

                    // map filepath to module
                    auto it = m_compilation_unit->m_imported_modules.find(canon_path);
                    if (it != m_compilation_unit->m_imported_modules.end()) {
                        it->second.push_back(module_declaration->GetModule());
                    } else {
                        m_compilation_unit->m_imported_modules[canon_path] = { module_declaration->GetModule() };
                    }
                }*/
            } else {
                // statement outside of module
            }
        }
    } else {
        AnalyzerInner();
    }
}

void SemanticAnalyzer::AnalyzerInner()
{
    Module *mod = m_compilation_unit->GetCurrentModule();
    while (m_ast_iterator->HasNext()) {
        m_ast_iterator->Next()->Visit(this, mod);
    }
}
