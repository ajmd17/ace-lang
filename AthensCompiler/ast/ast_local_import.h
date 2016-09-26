#ifndef AST_LOCAL_IMPORT_H
#define AST_LOCAL_IMPORT_H

#include "ast_import.h"

#include <string>

class AstLocalImport : public AstImport {
public:
    AstLocalImport(const std::string &path, const SourceLocation &location);

    inline const std::string &GetPath() const { return m_path; }

    virtual std::unique_ptr<Module> LoadModule(CompilationUnit *compilation_unit) const;

protected:
    std::string m_path;
};

#endif