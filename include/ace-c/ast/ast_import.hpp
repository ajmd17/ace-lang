#ifndef AST_IMPORT_HPP
#define AST_IMPORT_HPP

#include <ace-c/ast/ast_statement.hpp>
#include <ace-c/ast_iterator.hpp>
#include <ace-c/module.hpp>
#include <ace-c/compilation_unit.hpp>

#include <utility>
#include <memory>

class AstImport : public AstStatement {
public:
    AstImport(const SourceLocation &location);
    virtual ~AstImport() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;

protected:
    /** The AST iterator that will be used by the imported module */
    AstIterator m_ast_iterator;
};

#endif
