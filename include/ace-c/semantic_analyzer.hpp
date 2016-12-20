#ifndef SEMANTIC_ANALYZER_HPP
#define SEMANTIC_ANALYZER_HPP

#include <ace-c/ast_visitor.hpp>
#include <ace-c/ast/ast_declaration.hpp>

#include <memory>
#include <vector>
#include <string>

class SemanticAnalyzer : public AstVisitor {
public:
    SemanticAnalyzer(AstIterator *ast_iterator, CompilationUnit *compilation_unit);
    SemanticAnalyzer(const SemanticAnalyzer &other);

    /** Generates the compilation unit structure from the given statement iterator */
    void Analyze(bool expect_module_decl = true);

private:
    void AnalyzerInner();
};

#endif
