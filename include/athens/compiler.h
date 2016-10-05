#ifndef COMPILER_H
#define COMPILER_H

#include <athens/ast_visitor.h>

class Compiler : public AstVisitor {
public:
    Compiler(AstIterator *ast_iterator, CompilationUnit *compilation_unit);
    Compiler(const Compiler &other);

    void Compile();
};

#endif