#include <athens/semantic_analyzer.hpp>
#include <athens/ast/ast_module_declaration.hpp>
#include <athens/module.hpp>

SemanticAnalyzer::SemanticAnalyzer(AstIterator *ast_iterator, CompilationUnit *compilation_unit)
    : AstVisitor(ast_iterator, compilation_unit)
{
}

SemanticAnalyzer::SemanticAnalyzer(const SemanticAnalyzer &other)
    : AstVisitor(other.m_ast_iterator, other.m_compilation_unit)
{
}

void SemanticAnalyzer::Analyze()
{
    if (m_ast_iterator->HasNext()) {
        auto first_statement = m_ast_iterator->Next();
        auto module_declaration = std::dynamic_pointer_cast<AstModuleDeclaration>(first_statement);

        if (module_declaration != nullptr) {
            // all files must begin with a module declaration
            module_declaration->Visit(this, nullptr);
            m_compilation_unit->m_module_index++;

            Module *mod = m_compilation_unit->m_modules[m_compilation_unit->m_module_index].get();

            while (m_ast_iterator->HasNext()) {
                m_ast_iterator->Next()->Visit(this, mod);
            }

            // decrement the index to refer to the previous module
            m_compilation_unit->m_module_index--;
        }
    }
}
