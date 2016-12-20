#include <ace-c/compilation_unit.hpp>
#include <ace-c/configuration.hpp>

CompilationUnit::CompilationUnit()
    : m_module_index(0),
      m_global_module(new Module(ace::compiler::Config::GLOBAL_MODULE_NAME, SourceLocation::eof))
{
    m_module_tree.TopNode()->m_value = m_global_module.get();
}

Module *CompilationUnit::LookupModule(const std::string &name)
{
    TreeNode<Module*> *top = m_module_tree.TopNode();

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