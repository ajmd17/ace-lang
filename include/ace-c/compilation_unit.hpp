#ifndef COMPILATION_UNIT_HPP
#define COMPILATION_UNIT_HPP

#include <ace-c/module.hpp>
#include <ace-c/error_list.hpp>
#include <ace-c/emit/instruction_stream.hpp>

#include <vector>
#include <memory>
#include <string>

class CompilationUnit {
public:
    CompilationUnit();
    CompilationUnit(const CompilationUnit &other) = delete;

    inline std::unique_ptr<Module> &GetModule(int i) { return m_modules[i]; }
    inline const std::unique_ptr<Module> &GetModule(int i) const { return m_modules[i]; }
    inline std::unique_ptr<Module> &GetCurrentModule() { return m_modules[m_module_index]; }
    inline const std::unique_ptr<Module> &GetCurrentModule() const { return m_modules[m_module_index]; }
    inline ErrorList &GetErrorList() { return m_error_list; }
    inline const ErrorList &GetErrorList() const { return m_error_list; }
    inline InstructionStream &GetInstructionStream() { return m_instruction_stream; }
    inline const InstructionStream &GetInstructionStream() const { return m_instruction_stream; }

    /** all modules contained in the compilation unit */
    std::vector<std::unique_ptr<Module>> m_modules;
    /** the index of the current, active module in m_modules */
    int m_module_index;

private:
    ErrorList m_error_list;
    InstructionStream m_instruction_stream;
};

#endif
