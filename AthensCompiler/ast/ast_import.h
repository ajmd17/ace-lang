#ifndef AST_IMPORT_H
#define AST_IMPORT_H

#include "ast_statement.h"
#include "../module.h"
#include "../compilation_unit.h"

#include <memory>

class AstImport : public AstStatement {
public:
    AstImport(const SourceLocation &location);
    virtual ~AstImport() = default;

    virtual std::unique_ptr<Module> LoadModule(CompilationUnit *compilation_unit) const = 0;

    virtual void Visit(AstVisitor *visitor);
};

#endif