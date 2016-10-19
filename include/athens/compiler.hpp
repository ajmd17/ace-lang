#ifndef COMPILER_HPP
#define COMPILER_HPP

#include <athens/ast_visitor.hpp>

class Compiler : public AstVisitor {
public:
    Compiler(AstIterator *ast_iterator, CompilationUnit *compilation_unit);
    Compiler(const Compiler &other);

    void Compile();
};

#endif
