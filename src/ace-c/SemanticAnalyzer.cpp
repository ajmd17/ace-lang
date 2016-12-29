#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/ast/AstModuleDeclaration.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/ObjectType.hpp>

#include <common/my_assert.hpp>

#include <iostream>

IdentifierLookupResult SemanticAnalyzer::LookupIdentifier(AstVisitor *visitor, Module *mod, const std::string &name)
{
    IdentifierLookupResult res;
    res.type = IDENTIFIER_TYPE_UNKNOWN;

    const ObjectType *tmp = nullptr;

    // the variable must exist in the active scope or a parent scope
    if (res.as_identifier = mod->LookUpIdentifier(name, false)) {
        res.type = IDENTIFIER_TYPE_VARIABLE;
    } else if (res.as_identifier = visitor->GetCompilationUnit()->GetGlobalModule()->LookUpIdentifier(name, false)) {
        // if the identifier was not found,
        // look in the global module to see if it is a global function.
        res.type = IDENTIFIER_TYPE_VARIABLE;
    } else if (res.as_module = visitor->GetCompilationUnit()->LookupModule(name)) {
        res.type = IDENTIFIER_TYPE_MODULE;
    } else if (tmp = ObjectType::GetBuiltinType(name)) {
        res.as_type = *tmp;
        res.type = IDENTIFIER_TYPE_TYPE;
    } else if (mod->LookUpUserType(name, res.as_type)) {
        res.type = IDENTIFIER_TYPE_TYPE;
    } else if (visitor->GetCompilationUnit()->GetGlobalModule()->LookUpUserType(name, res.as_type)) {
        res.type = IDENTIFIER_TYPE_TYPE;
    } else {
        // nothing was found
        res.type = IDENTIFIER_TYPE_NOT_FOUND;
    }
    
    return res;
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
