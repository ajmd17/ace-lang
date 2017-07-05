#include <ace-c/ast/AstFileImport.hpp>
#include <ace-c/SourceFile.hpp>
#include <ace-c/Lexer.hpp>
#include <ace-c/Parser.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/Optimizer.hpp>

#include <common/str_util.hpp>

#include <fstream>
#include <iostream>
#include <functional>

AstFileImport::AstFileImport(
    const std::string &path,
    const SourceLocation &location)
    : AstImport(location),
      m_path(path)
{
}

void AstFileImport::Visit(AstVisitor *visitor, Module *mod)
{
    // find the folder which the current file is in
    std::string dir;
    const size_t index = m_location.GetFileName().find_last_of("/\\");
    if (index != std::string::npos) {
        dir = m_location.GetFileName().substr(0, index) + "/";
    }

    // create relative path
    std::string filepath = dir + m_path;

    AstImport::PerformImport(
        visitor,
        mod,
        filepath
    );
}

Pointer<AstStatement> AstFileImport::Clone() const
{
    return CloneImpl();
}
