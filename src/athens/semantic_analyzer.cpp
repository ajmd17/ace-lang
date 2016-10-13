#include <athens/semantic_analyzer.h>
#include <athens/ast/ast_module_declaration.h>

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
    SourceLocation empty_location(0, 0, "");

    if (Assert(m_ast_iterator->HasNext(),
        CompilerError(Level_fatal, Msg_empty_module, empty_location))) {

        auto first_statement = m_ast_iterator->Next();
        auto module_declaration = std::dynamic_pointer_cast<AstModuleDeclaration>(first_statement);

        if (Assert(module_declaration != nullptr,
            CompilerError(Level_fatal, Msg_expected_module,
                first_statement != nullptr ? first_statement->GetLocation() : empty_location))) {

            // all files must begin with a module declaration
            module_declaration->Visit(this);
            m_compilation_unit->m_module_index++;
            
            // open global scope
            m_compilation_unit->CurrentModule()->m_scopes.Open(Scope());

            while (m_ast_iterator->HasNext()) {
                m_ast_iterator->Next()->Visit(this);
            }

            // close global scope
            m_compilation_unit->CurrentModule()->m_scopes.Close();

            // decrement the index to refer to the previous module
            m_compilation_unit->m_module_index--;
        }
    }
}