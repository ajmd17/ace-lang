#ifndef COMPILATION_UNIT_HPP
#define COMPILATION_UNIT_HPP

#include <ace-c/module.hpp>
#include <ace-c/error_list.hpp>
#include <ace-c/emit/instruction_stream.hpp>
#include <ace-c/tree.hpp>

#include <vector>
#include <memory>
#include <string>

class CompilationUnit {
public:
    CompilationUnit();
    CompilationUnit(const CompilationUnit &other) = delete;

    inline std::shared_ptr<Module> &GetGlobalModule() { return m_module_tree.Root(); /*return m_modules[0];*/ }
    inline const std::shared_ptr<Module> &GetGlobalModule() const { return m_module_tree.Root();/*return m_modules[0];*/ }

    //inline std::shared_ptr<Module> &GetModule(int i) { return m_modules[i]; }
    //inline const std::unique_ptr<Module> &GetModule(int i) const { return m_modules[i]; }

    inline std::shared_ptr<Module> &GetCurrentModule() { return m_module_tree.Top();/*return m_modules[m_module_index];*/ }
    inline const std::shared_ptr<Module> &GetCurrentModule() const { return m_module_tree.Top();/*return m_modules[m_module_index];*/ }

    inline ErrorList &GetErrorList() { return m_error_list; }
    inline const ErrorList &GetErrorList() const { return m_error_list; }

    inline InstructionStream &GetInstructionStream() { return m_instruction_stream; }
    inline const InstructionStream &GetInstructionStream() const { return m_instruction_stream; }

    /** Looks up the module with the name, taking scope into account.
        Modules with the name that are in the current module or any module
        above the current one will be considered.
    */
    std::shared_ptr<Module> LookupModule(const std::string &name);

    Tree<std::shared_ptr<Module>> m_module_tree;

    /** all modules contained in the compilation unit */
    //std::vector<std::unique_ptr<Module>> m_modules;
    /** the index of the current, active module in m_modules */
    int m_module_index;

private:
    ErrorList m_error_list;
    InstructionStream m_instruction_stream;
};

#endif
