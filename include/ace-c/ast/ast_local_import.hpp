#ifndef AST_LOCAL_IMPORT_HPP
#define AST_LOCAL_IMPORT_HPP

#include <ace-c/ast/ast_import.hpp>

#include <string>

class AstLocalImport : public AstImport {
public:
    AstLocalImport(const std::string &path, const SourceLocation &location);

    inline const std::string &GetPath() const { return m_path; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;

protected:
    std::string m_path;
};

#endif
