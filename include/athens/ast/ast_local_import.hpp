#ifndef AST_LOCAL_IMPORT_HPP
#define AST_LOCAL_IMPORT_HPP

#include <athens/ast/ast_import.hpp>

#include <string>

class AstLocalImport : public AstImport {
public:
    AstLocalImport(const std::string &path, const SourceLocation &location);

    inline const std::string &GetPath() const { return m_path; }

    virtual void Visit(AstVisitor *visitor) override;

protected:
    std::string m_path;
};

#endif
