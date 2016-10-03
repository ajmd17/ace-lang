#ifndef AST_IMPORT_H
#define AST_IMPORT_H

#include <athens/ast/ast_statement.h>
#include <athens/ast_iterator.h>
#include <athens/module.h>
#include <athens/compilation_unit.h>

#include <utility>
#include <memory>

class AstImport : public AstStatement {
public:
    AstImport(const SourceLocation &location);
    virtual ~AstImport() = default;

    virtual std::unique_ptr<Module>
        LoadModule(CompilationUnit *compilation_unit) = 0;

    virtual void Visit(AstVisitor *visitor);
    virtual void Build(AstVisitor *visitor) const;
    virtual void Optimize(AstVisitor *visitor);

protected:
    /** The AST iterator that will be used by the imported module */
    AstIterator m_ast_iterator;
};

#endif