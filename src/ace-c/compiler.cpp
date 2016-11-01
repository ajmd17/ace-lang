#include <ace-c/compiler.hpp>
#include <ace-c/module.hpp>
#include <ace-c/ast/ast_module_declaration.hpp>

Compiler::Compiler(AstIterator *ast_iterator, CompilationUnit *compilation_unit)
    : AstVisitor(ast_iterator, compilation_unit)
{
}

Compiler::Compiler(const Compiler &other)
    : AstVisitor(other.m_ast_iterator, other.m_compilation_unit)
{
}

void Compiler::Compile()
{
    if (m_ast_iterator->HasNext()) {
        auto first_statement = m_ast_iterator->Next();
        auto module_declaration = std::dynamic_pointer_cast<AstModuleDeclaration>(first_statement);

        if (module_declaration != nullptr) {
            // all files must begin with a module declaration
            module_declaration->Build(this, nullptr);
            m_compilation_unit->m_module_index++;

            Module *mod = m_compilation_unit->m_modules[m_compilation_unit->m_module_index].get();

            while (m_ast_iterator->HasNext()) {
                m_ast_iterator->Next()->Build(this, mod);
            }

            // decrement the index to refer to the previous module
            m_compilation_unit->m_module_index--;
        }
    }
}
