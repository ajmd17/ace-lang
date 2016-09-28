#ifndef AST_IMPORT_H
#define AST_IMPORT_H

#include <athens/ast/ast_statement.h>
#include <athens/module.h>
#include <athens/compilation_unit.h>

#include <memory>

class AstImport : public AstStatement {
public:
    AstImport(const SourceLocation &location);
    virtual ~AstImport() = default;

    virtual std::unique_ptr<Module> LoadModule(CompilationUnit *compilation_unit) const = 0;

    virtual void Visit(AstVisitor *visitor);
    virtual void Build(AstVisitor *visitor) const;
    virtual void Optimize() = 0;
};

#endif