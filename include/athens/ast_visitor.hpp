#ifndef AST_VISITOR_HPP
#define AST_VISITOR_HPP

#include <athens/ast_iterator.hpp>
#include <athens/error_list.hpp>
#include <athens/compiler_error.hpp>
#include <athens/compilation_unit.hpp>

class AstVisitor {
public:
    AstVisitor(AstIterator *ast_iterator, CompilationUnit *compilation_unit);
    virtual ~AstVisitor() = default;

    inline AstIterator *GetIterator() const { return m_ast_iterator; }
    inline CompilationUnit *GetCompilationUnit() const { return m_compilation_unit; }

    /** If expr is false, the given error is added to the error list. */
    bool Assert(bool expr, const CompilerError &error);

protected:
    AstIterator *m_ast_iterator;
    CompilationUnit *m_compilation_unit;
};

#endif
