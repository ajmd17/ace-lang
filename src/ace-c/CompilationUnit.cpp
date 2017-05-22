#include <ace-c/CompilationUnit.hpp>
#include <ace-c/Configuration.hpp>
#include <ace-c/emit/StaticObject.hpp>
#include <ace-c/emit/NamesPair.hpp>
#include <ace-c/Configuration.hpp>

#include <common/my_assert.hpp>

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
    top.GetIdentifierTable().AddSymbolType(SymbolType::Builtin::EVENT);
    top.GetIdentifierTable().AddSymbolType(SymbolType::Builtin::EVENT_ARRAY);

    m_module_tree.TopNode()->m_value = m_global_module.get();
}

CompilationUnit::~CompilationUnit()
{
    m_global_module->m_scopes.Close();
}

void CompilationUnit::RegisterType(SymbolTypePtr_t &type_ptr)
{
    std::vector<NamesPair_t> names;

    for (auto &mem : type_ptr->GetMembers()) {
        std::string mem_name = std::get<0>(mem);

        names.push_back({
            mem_name.size(),
            std::vector<uint8_t>(mem_name.begin(), mem_name.end())
        });
    }

    // mangle the type name
    size_t len = type_ptr->GetName().length();

    ASSERT(type_ptr->GetMembers().size() < ace::compiler::Config::MAX_DATA_MEMBERS);

    // create static object
    StaticTypeInfo st;
    st.m_size = type_ptr->GetMembers().size();
    st.m_names = names;
    st.m_name = new char[len + 1];
    st.m_name[len] = '\0';
    std::strcpy(st.m_name, type_ptr->GetName().c_str());

    int id;

    StaticObject so(st);
    int found_id = m_instruction_stream.FindStaticObject(so);
    if (found_id == -1) {
        so.m_id = m_instruction_stream.NewStaticId();
        m_instruction_stream.AddStaticObject(so);
        id = so.m_id;
    } else {
        id = found_id;
    }

    delete[] st.m_name;

    type_ptr->SetId(id);
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