#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/ast/AstModuleDeclaration.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/SymbolType.hpp>

#include <common/my_assert.hpp>

#include <set>
#include <iostream>

IdentifierLookupResult SemanticAnalyzer::LookupIdentifier(
    AstVisitor *visitor,
    Module *mod,
    const std::string &name)
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
        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
            LEVEL_ERROR,
            Msg_arg_type_incompatible,
            location,
            arg_type->GetName(),
            param_type->GetName()
        ));
    }
}

struct ArgInfo {
    bool is_named;
    std::string name;
    SymbolTypePtr_t type;
};

static int FindFreeSlot(
    int current_index,
    const std::set<int> &used_indices,
    const std::vector<GenericInstanceTypeInfo::Arg> &generic_args,
    bool is_variadic = false,
    int num_supplied_args = -1)
{
    size_t num_params = generic_args.size() - 1; // - 1 for return type
    size_t counter = 0;
    
    while (counter < num_params) {
        if (current_index == num_params) {
            current_index = 0;
        }

        // check if index is used
        if (used_indices.find(current_index) == used_indices.end()) {
            // not found, return the index
            return current_index;
        }

        current_index++;
        counter++;
    }

    // no slot available
    return is_variadic
        ? current_index
        : -1;
}

static int ArgIndex(
    int current_index,
    const ArgInfo &arg_info,
    const std::set<int> &used_indices,
    const std::vector<GenericInstanceTypeInfo::Arg> &generic_args,
    bool is_variadic = false,
    int num_supplied_args = -1)
{
    if (arg_info.is_named) {
        for (int j = 1; j < generic_args.size(); j++) {
            const std::string &generic_arg_name = generic_args[j].m_name;
            if (generic_arg_name == arg_info.name) {
                if (used_indices.find(j - 1) == used_indices.end()) {
                    return j - 1; // subtract 1 because first param will be return type
                }
            }
        }
        return -1;
    }

    return FindFreeSlot(
        current_index,
        used_indices,
        generic_args,
        is_variadic,
        num_supplied_args
    );
}

std::pair<SymbolTypePtr_t, std::vector<std::shared_ptr<AstArgument>>> SemanticAnalyzer::SubstituteFunctionArgs(
    AstVisitor *visitor, Module *mod, 
    const SymbolTypePtr_t &identifier_type, 
    const std::vector<std::shared_ptr<AstArgument>> &args,
    const SourceLocation &location)
{
    std::vector<std::shared_ptr<AstArgument>> res_args;
    res_args.resize(args.size());

    if (identifier_type->GetTypeClass() == TYPE_GENERIC_INSTANCE) {
        const SymbolTypePtr_t base = identifier_type->GetBaseType();

        if (base == SymbolType::Builtin::FUNCTION) {
            // the indices of the arguments (will be returned)
            std::vector<int> substituted_param_ids;

            const auto &generic_args = identifier_type->GetGenericInstanceInfo().m_generic_args;

            // check for varargs (at end)
            bool is_varargs = false;
            SymbolTypePtr_t vararg_type;

            if (!generic_args.empty()) {
                const SymbolTypePtr_t &last_generic_arg_type = generic_args.back().m_type;
                ASSERT(last_generic_arg_type != nullptr);

                if (last_generic_arg_type->GetTypeClass() == TYPE_GENERIC_INSTANCE) {
                    const SymbolTypePtr_t arg_base = last_generic_arg_type->GetBaseType();
                    // check if it is an instance of varargs type
                    if (arg_base == SymbolType::Builtin::VAR_ARGS) {
                        is_varargs = true;

                        ASSERT(!last_generic_arg_type->GetGenericInstanceInfo().m_generic_args.empty());
                        vararg_type = last_generic_arg_type->GetGenericInstanceInfo().m_generic_args[0].m_type;
                    }
                }
            }

            std::set<int> used_indices;

            /*for (size_t i = 0; i < args.size(); i++) {
                ASSERT(args[i] != nullptr);

                ArgInfo arg_info;
                arg_info.is_named = args[i]->IsNamed();
                arg_info.name = args[i]->GetName();
                arg_info.type = args[i]->GetSymbolType();

                const int found_index = ArgIndex(
                    i, used_indices, generic_args, arg_info
                );

                if (found_index != -1) {
                    used_indices.insert(found_index);
                }

                std::cout << "arg index for " << arg_info.name << "(" << i << ") = " << found_index << "\n";
            }*/

            // add default params
            /*for (size_t i = 1; i < generic_args.size(); i++) {
                const auto &param = generic_args[i];
                bool found_sub = false;

                for (size_t j = 0; j < args.size(); j++) {
                    const auto &arg = args[j];
                    ASSERT(arg != nullptr);

                    if (arg->IsNamed()) {
                        if (arg->GetName() == param.m_name) {
                            // found, break out
                            found_sub = true;
                            break;
                        }
                    }
                }

                if (!found_sub) {
                    // still in index, use the one at the index
                }
            }*/

            if (generic_args.size() == args.size() + 1 ||
                (is_varargs && generic_args.size() - 1 < args.size() + 2))
            {
                using ArgDataPair = std::pair<ArgInfo, std::shared_ptr<AstArgument>>;

                std::vector<ArgDataPair> named_args;
                std::vector<ArgDataPair> unnamed_args;

                // find named args first
                for (int i = 0; i < args.size(); i++) {
                    ASSERT(args[i] != nullptr);

                    ArgInfo arg_info;
                    arg_info.is_named = args[i]->IsNamed();
                    arg_info.name = args[i]->GetName();
                    arg_info.type = args[i]->GetSymbolType();

                    ArgDataPair arg_data_pair = { arg_info, args[i] };

                    if (arg_info.is_named) {
                        named_args.push_back(arg_data_pair);
                    } else {
                        unnamed_args.push_back(arg_data_pair);
                    }
                }

                // handle named arguments first
                for (size_t i = 0; i < named_args.size(); i++) {
                    const ArgDataPair &arg = named_args[i];

                    const int found_index = ArgIndex(
                        i,
                        std::get<0>(arg),
                        used_indices,
                        generic_args
                    );

                    if (found_index != -1) {
                        used_indices.insert(found_index);

                        // found successfully, check type compatibility
                        const SymbolTypePtr_t &param_type = generic_args[found_index + 1].m_type;

                        CheckArgTypeCompatible(
                            visitor,
                            std::get<1>(arg)->GetLocation(),
                            std::get<0>(arg).type,
                            param_type
                        );

                        res_args[found_index] = std::get<1>(arg);
                        std::cout << "arg index for " << std::get<0>(arg).name << "(" << i << ") = " << found_index << "\n";
                    } else {
                        // not found so add error
                        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                            LEVEL_ERROR,
                            Msg_named_arg_not_found,
                            std::get<1>(arg)->GetLocation(),
                            std::get<0>(arg).name
                        ));
                    }
                }

                // handle unnamed arguments
                for (size_t i = 0; i < unnamed_args.size(); i++) {
                    const ArgDataPair &arg = unnamed_args[i];

                    const int found_index = ArgIndex(
                        i,
                        std::get<0>(arg),
                        used_indices,
                        generic_args,
                        is_varargs
                    );

                    if (found_index != -1) {
                        used_indices.insert(found_index);
                    }
                    
                    if (is_varargs && ((i + named_args.size()) + 2) >= generic_args.size()) {
                        // in varargs... check against vararg base type
                        CheckArgTypeCompatible(
                            visitor,
                            std::get<1>(arg)->GetLocation(),
                            std::get<0>(arg).type,
                            vararg_type
                        );

                        if (found_index >= res_args.size()) {
                            // at end, push
                            res_args.push_back(std::get<1>(arg));
                        } else {
                            res_args[found_index] = std::get<1>(arg);
                        }
                    } else {
                        if (found_index == -1) {
                            // too many args supplied
                            visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                                LEVEL_ERROR,
                                Msg_too_many_args,
                                location,
                                generic_args.size() - 1,
                                args.size()
                            ));
                        } else {
                            // choose whether to get next param, or last (in the case of varargs)
                            struct {
                                std::string name;
                                SymbolTypePtr_t type;    
                            } param_info;

                            const GenericInstanceTypeInfo::Arg &param = (found_index + 1 < generic_args.size())
                                ? generic_args[found_index + 1]
                                : generic_args.back();
                            
                            param_info.name = param.m_name;
                            param_info.type = param.m_type;

                            CheckArgTypeCompatible(
                                visitor,
                                std::get<1>(arg)->GetLocation(),
                                std::get<0>(arg).type,
                                param_info.type
                            );

                            res_args[found_index] = std::get<1>(arg);
                        }
                    }
                }
            } else {
                // wrong number of args given
                ErrorMessage msg = (args.size() + 1 > generic_args.size())
                    ? Msg_too_many_args
                    : Msg_too_few_args;
                
                visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
                    LEVEL_ERROR,
                    msg,
                    location,
                    is_varargs
                        ? generic_args.size() - 2
                        : generic_args.size() - 1,
                    args.size()
                ));
            }

            // make sure the "return type" of the function is not null
            ASSERT(generic_args.size() >= 1);
            ASSERT(generic_args[0].m_type != nullptr);

            // return the "return type" of the function
            return {
                generic_args[0].m_type,
                res_args
                //substituted_param_ids
            };
        }
    } else if (identifier_type == SymbolType::Builtin::FUNCTION ||
               identifier_type == SymbolType::Builtin::ANY)
    {
        // the indices of the arguments (will be returned)
        std::vector<int> substituted_param_ids;

        for (size_t i = 0; i < args.size(); i++) {
            substituted_param_ids.push_back(i);
        }

        // abstract function, allow any params
        return {
            SymbolType::Builtin::ANY,
            args
        };
    }
    
    return {
        nullptr,
        args
    };
}

SemanticAnalyzer::SemanticAnalyzer(
    AstIterator *ast_iterator,
    CompilationUnit *compilation_unit)
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
            auto first_stmt = m_ast_iterator->Next();
            if (AstModuleDeclaration *mod_decl = dynamic_cast<AstModuleDeclaration*>(first_stmt.get())) {
                mod_decl->Visit(this, nullptr);
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
