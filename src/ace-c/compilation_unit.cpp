#include <ace-c/compilation_unit.hpp>
#include <ace-c/configuration.hpp>

CompilationUnit::CompilationUnit()
    : m_module_index(0)
{
    // create global module
    m_modules.push_back(std::unique_ptr<Module>(
        new Module(ace::compiler::Config::GLOBAL_MODULE_NAME, SourceLocation::eof)));
}
