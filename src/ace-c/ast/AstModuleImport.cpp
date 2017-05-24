#include <ace-c/ast/AstModuleImport.hpp>
#include <ace-c/SourceFile.hpp>
#include <ace-c/Lexer.hpp>
#include <ace-c/Parser.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/Optimizer.hpp>
#include <ace-c/Minifier.hpp>

#include <common/str_util.hpp>

#include <fstream>
#include <iostream>
#include <functional>

AstModuleImport::AstModuleImport(
    const std::string &mod_name,
    const std::shared_ptr<AstModuleImport> &right,
    const SourceLocation &location)
    : AstImport(location),
      m_mod_name(mod_name),
      m_right(right)
{
}

void AstModuleImport::Visit(AstVisitor *visitor, Module *mod)
{
    
    // find the folder which the current file is in
    std::string current_dir;
    const size_t index = m_location.GetFileName().find_last_of("/\\");
    if (index != std::string::npos) {
        current_dir = m_location.GetFileName().substr(0, index) + "/";
    }

    std::ifstream file;
    std::string found_path;
    bool opened = false;

    // iterate through library paths to try and find a file
    for (const std::string &scan_path : mod->GetScanPaths()) {
      // create relative path
      const std::string path = current_dir + scan_path + "/";
      const std::string &filename = m_mod_name;
      const std::string ext = ".ace";

      found_path = path + filename + ext;
      std::cout << "trying path: " << found_path << "\n";
      if (AstImport::TryOpenFile(found_path, file)) {
          opened = true;
          break;
      }

      // try it without extension
      found_path = path + filename;
      std::cout << "trying path: " << found_path << "\n";
      if (AstImport::TryOpenFile(found_path, file)) {
          opened = true;
          break;
      }
    }

    if (opened) {
        AstImport::PerformImport(
            visitor,
            mod,
            found_path
        );

        if (m_right != nullptr) {
            if (Module *this_module = visitor->GetCompilationUnit()->LookupModule(m_mod_name)) {
            }
        }
    } else {
        visitor->GetCompilationUnit()->GetErrorList().AddError(CompilerError(
            LEVEL_ERROR,
            Msg_could_not_find_module,
            m_location,
            m_mod_name
        ));
    }
}

void AstModuleImport::Recreate(std::ostringstream &ss)
{
    m_ast_iterator.ResetPosition();
    Minifier minifier(&m_ast_iterator);
    minifier.Minify(ss);
}

Pointer<AstStatement> AstModuleImport::Clone() const
{
    return CloneImpl();
}