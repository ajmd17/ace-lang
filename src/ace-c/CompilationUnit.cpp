#include <ace-c/CompilationUnit.hpp>
#include <ace-c/Configuration.hpp>

CompilationUnit::CompilationUnit()
    : m_module_index(0),
      m_global_module(new Module(ace::compiler::Config::GLOBAL_MODULE_NAME, SourceLocation::eof))
{
    m_global_module->m_scopes.Open(Scope());

    Scope &top = m_global_module->m_scopes.Top();
    top.GetIdentifierTable().AddSymbolType(SymbolType::Builtin::ANY);
    top.GetIdentifierTable().AddSymbolType(SymbolType::Builtin::INT);
    top.GetIdentifierTable().AddSymbolType(SymbolType::Builtin::FLOAT);
    top.GetIdentifierTable().AddSymbolType(SymbolType::Builtin::NUMBER);
    top.GetIdentifierTable().AddSymbolType(SymbolType::Builtin::BOOLEAN);
    top.GetIdentifierTable().AddSymbolType(SymbolType::Builtin::STRING);
    top.GetIdentifierTable().AddSymbolType(SymbolType::Builtin::FUNCTION);
    top.GetIdentifierTable().AddSymbolType(SymbolType::Builtin::ARRAY);
    top.GetIdentifierTable().AddSymbolType(SymbolType::Builtin::MAYBE);
    top.GetIdentifierTable().AddSymbolType(SymbolType::Builtin::NULL_TYPE);

    m_module_tree.TopNode()->m_value = m_global_module.get();
}

CompilationUnit::~CompilationUnit()
{
    m_global_module->m_scopes.Close();
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