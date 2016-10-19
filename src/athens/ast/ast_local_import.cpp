#include <athens/ast/ast_local_import.hpp>
#include <athens/source_file.hpp>
#include <athens/lexer.hpp>
#include <athens/parser.hpp>
#include <athens/semantic_analyzer.hpp>
#include <athens/optimizer.hpp>

#include <fstream>
#include <iostream>

AstLocalImport::AstLocalImport(const std::string &path, const SourceLocation &location)
    : AstImport(location),
      m_path(path)
{
}

void AstLocalImport::Visit(AstVisitor *visitor)
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

    if (!file.is_open()) {
        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
            Level_fatal, Msg_could_not_open_file, m_location, filepath));
    } else {
        // get number of bytes
        size_t max = file.tellg();
        // seek to beginning
        file.seekg(0, std::ios::beg);
        // load stream into file buffer
        SourceFile source_file(filepath, max);
        file.read(source_file.GetBuffer(), max);

        // use the lexer and parser on this file buffer
        TokenStream token_stream;
        Lexer lexer(SourceStream(&source_file), &token_stream, visitor->GetCompilationUnit());
        lexer.Analyze();

        Parser parser(&m_ast_iterator, &token_stream, visitor->GetCompilationUnit());
        parser.Parse();

        SemanticAnalyzer semantic_analyzer(&m_ast_iterator, visitor->GetCompilationUnit());
        semantic_analyzer.Analyze();
    }
}
