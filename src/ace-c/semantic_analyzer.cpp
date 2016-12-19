#include <ace-c/semantic_analyzer.hpp>
#include <ace-c/ast/ast_module_declaration.hpp>
#include <ace-c/module.hpp>

#include <common/my_assert.hpp>

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
        if (m_ast_iterator->HasNext()) {
            auto first_statement = m_ast_iterator->Next();
            auto module_declaration = std::dynamic_pointer_cast<AstModuleDeclaration>(first_statement);

            if (module_declaration) {
                // all files must begin with a module declaration
                module_declaration->Visit(this, nullptr);
                AnalyzerInner();
            }
        }
    } else {
        AnalyzerInner();
    }
}

void SemanticAnalyzer::AnalyzerInner()
{
    Module *mod = m_compilation_unit->GetCurrentModule().get();
    
    // visit all built-in objects defined
    for (auto &decl : m_decls) {
        decl->Visit(this, mod);
    }

    while (m_ast_iterator->HasNext()) {
        m_ast_iterator->Next()->Visit(this, mod);
    }
}
