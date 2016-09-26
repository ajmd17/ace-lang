#ifndef COMPILATION_UNIT_H
#define COMPILATION_UNIT_H

#include "module.h"
#include "error_list.h"
#include "emit/instruction_stream.h"

#include <list>
#include <vector>
#include <memory>

class CompilationUnit {
public:
    CompilationUnit();
    CompilationUnit(const CompilationUnit &other) = delete;

    inline ErrorList &GetErrorList() { return m_error_list; }
    inline const ErrorList &GetErrorList() const { return m_error_list; }
    inline InstructionStream &GetInstructionStream() { return m_instruction_stream; }
    inline const InstructionStream &GetInstructionStream() const { return m_instruction_stream; }
    inline std::unique_ptr<Module> &CurrentModule() { return m_modules[m_module_index]; }
    inline const std::unique_ptr<Module> &CurrentModule() const { return m_modules[m_module_index]; }

    /** all modules contained in the compilation unit */
    std::vector<std::unique_ptr<Module>> m_modules;
    /** the index of the current, active module in m_modules */
    size_t m_module_index;

private:
    ErrorList m_error_list;
    InstructionStream m_instruction_stream;
};

#endif