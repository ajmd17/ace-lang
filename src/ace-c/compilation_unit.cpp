#include <ace-c/compilation_unit.hpp>

CompilationUnit::CompilationUnit()
    : m_module_index(0)
{
    // create global module
    m_modules.push_back(std::unique_ptr<Module>(new Module("__global__", SourceLocation::eof)));
}
