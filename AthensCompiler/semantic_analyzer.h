#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "ast_visitor.h"
#include "module.h"
#include "compiler_error.h"
#include "error_list.h"

#include <memory>

class SemanticAnalyzer : public AstVisitor {
public:
    SemanticAnalyzer(const AstIterator &ast_iterator, CompilationUnit *compilation_unit);
    SemanticAnalyzer(const SemanticAnalyzer &other);

    /** Generates the compilation unit structure from the given statement iterator */
    void Analyze();
};

#endif