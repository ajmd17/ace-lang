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

SymbolTypePtr_t SemanticAnalyzer::SubstituteFunctionArgs(AstVisitor *visitor, Module *mod, 
    const SymbolTypePtr_t &identifier_type, 
    const std::vector<std::shared_ptr<AstExpression>> &args,
    const SourceLocation &location)
{
    if (identifier_type->GetTypeClass() == TYPE_GENERIC_INSTANCE) {
        auto base = identifier_type->GetBaseType();

        if (base == SymbolType::Builtin::FUNCTION) {

            auto &param_types = identifier_type->GetGenericInstanceInfo().m_param_types;
            // check for varargs (at end)
            bool is_varargs = false;
            SymbolTypePtr_t vararg_type;
            if (!param_types.empty() && param_types.back()->GetTypeClass() == TYPE_GENERIC_INSTANCE) {
                auto base = param_types.back()->GetBaseType();
                if (base == SymbolType::Builtin::VAR_ARGS) {
                    is_varargs = true;
                    ASSERT(!param_types.back()->GetGenericInstanceInfo().m_param_types.empty());
                    vararg_type = param_types.back()->GetGenericInstanceInfo().m_param_types[0];
                }
            }

            if (param_types.size() == args.size() + 1 || (is_varargs && param_types.size() - 1 < args.size() + 2)) {
                for (int i = 0; i < args.size(); i++) {
                    SymbolTypePtr_t arg_type =
                        args[i]->GetSymbolType();

                    const SymbolTypePtr_t &param_type = i + 1 < param_types.size()
                        ? param_types[i + 1]
                        : param_types.back();

                    ASSERT(arg_type != nullptr);
                    ASSERT(param_type != nullptr);

                    if (is_varargs && i + 2 >= param_types.size()) {
                        // in varargs... check vararg base type
                        ASSERT(vararg_type != nullptr);

                        if (!vararg_type->TypeCompatible(*arg_type, true)) {
                            visitor->GetCompilationUnit()->GetErrorList().AddError(
                                CompilerError(Level_fatal, Msg_arg_type_incompatible, 
                                    args[i]->GetLocation(), arg_type->GetName(), vararg_type->GetName()));
                        }
                    } else {
                        // make sure argument types are compatible
                        // use strict numbers so that floats cannot be passed as explicit ints
                        if (!param_type->TypeCompatible(*arg_type, true)) {
                            visitor->GetCompilationUnit()->GetErrorList().AddError(
                                CompilerError(Level_fatal, Msg_arg_type_incompatible, 
                                    args[i]->GetLocation(), arg_type->GetName(), param_type->GetName()));
                        }
                    }
                }
            } else {
                // wrong number of args given
                ErrorMessage msg = args.size() + 1 > identifier_type->GetGenericInstanceInfo().m_param_types.size()
                    ? Msg_too_many_args : Msg_too_few_args;
                
                visitor->GetCompilationUnit()->GetErrorList().AddError(
                    CompilerError(Level_fatal, msg, location));
            }

            ASSERT(identifier_type->GetGenericInstanceInfo().m_param_types.size() >= 1);
            ASSERT(identifier_type->GetGenericInstanceInfo().m_param_types[0] != nullptr);

            return identifier_type->GetGenericInstanceInfo().m_param_types[0];
        }
    } else if (identifier_type == SymbolType::Builtin::FUNCTION) {
        // abstract function, allow any params
        return SymbolType::Builtin::ANY;
    }
    
    return nullptr;
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
