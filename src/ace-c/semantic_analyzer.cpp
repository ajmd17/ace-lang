#include <ace-c/semantic_analyzer.hpp>
#include <ace-c/ast/ast_module_declaration.hpp>
#include <ace-c/module.hpp>

#include <common/my_assert.hpp>

#include <iostream>

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
