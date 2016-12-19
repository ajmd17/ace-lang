#include <ace-c/compilation_unit.hpp>
#include <ace-c/configuration.hpp>

CompilationUnit::CompilationUnit()
    : m_module_index(0)
{
    // set top to be global module
    m_module_tree.Top().reset(
        new Module(ace::compiler::Config::GLOBAL_MODULE_NAME, SourceLocation::eof));
}

std::shared_ptr<Module> CompilationUnit::LookupModule(const std::string &name)
{
    TreeNode<std::shared_ptr<Module>> *top = m_module_tree.TopNode();

    while (top) {
        if (top->m_value && top->m_value->GetName() == name) {
            return top->m_value;
        }

        // look up module names in the top module's siblings
        for (auto &sibling : top->m_siblings) {
            if (sibling && sibling->m_value) {
                if (sibling->m_value->GetName() == name) {
                    return sibling->m_value;
                }
            }
        }

        top = top->m_parent;
    }

    return nullptr;
}