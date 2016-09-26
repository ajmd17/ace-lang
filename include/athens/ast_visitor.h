#ifndef AST_VISITOR_H
#define AST_VISITOR_H

#include <athens/ast_iterator.h>
#include <athens/error_list.h>
#include <athens/compiler_error.h>
#include <athens/compilation_unit.h>

class AstVisitor {
public:
    AstVisitor(const AstIterator &ast_iterator, CompilationUnit *compilation_unit);
    virtual ~AstVisitor() = default;

    inline AstIterator &GetIterator() { return m_ast_iterator; }
    inline const AstIterator &GetIterator() const { return m_ast_iterator; }
    inline CompilationUnit *GetCompilationUnit() const { return m_compilation_unit; }

    /** If expr is false, the given error is added to the error list. */
    bool Assert(bool expr, const CompilerError &error);

protected:
    AstIterator m_ast_iterator;
    CompilationUnit *m_compilation_unit;
};

#endif