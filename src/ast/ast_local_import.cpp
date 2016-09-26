#include <athens/ast/ast_local_import.h>
#include <athens/source_file.h>

#include <fstream>

AstLocalImport::AstLocalImport(const std::string &path, const SourceLocation &location)
    : m_path(path), 
      AstImport(location)
{
}

std::unique_ptr<Module> AstLocalImport::LoadModule(CompilationUnit *compilation_unit) const
{
    // find the folder which the current file is in
    std::string dir;
    size_t index = m_location.GetFileName().find_last_of("/\\");
    if (index != std::string::npos) {
        dir = m_location.GetFileName().substr(0, index) + "/";
    }
    
    // create relative path
    std::string filepath(dir + m_path);

    std::ifstream file(filepath, std::ios::in | std::ios::ate);
    if (!file.good()) {
        compilation_unit->GetErrorList().AddError(CompilerError(
            Level_fatal, Msg_could_not_open_file, m_location, filepath));
    } else {
        // get number of bytes
        size_t max = file.tellg();
        // seek to beginning
        file.seekg(0, std::ios::beg);
        // load stream into file buffer
        SourceFile source_file(max);
        file.read(source_file.GetBuffer(), max);

        // Todo: use the lexer and parser on this file buffer
    }

    return nullptr;
}