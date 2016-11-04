#ifndef SEMANTIC_ANALYZER_HPP
#define SEMANTIC_ANALYZER_HPP

#include <ace-c/ast_visitor.hpp>
#include <ace-c/ast/ast_declaration.hpp>

#include <memory>
#include <vector>

class SemanticAnalyzer : public AstVisitor {
public:
    SemanticAnalyzer(AstIterator *ast_iterator, CompilationUnit *compilation_unit);
    SemanticAnalyzer(const SemanticAnalyzer &other);

    inline void AddBuiltinObject(const std::shared_ptr<AstDeclaration> &decl)
        { m_decls.push_back(decl); }

    /** Generates the compilation unit structure from the given statement iterator */
    void Analyze(bool expect_module_decl = true);

private:
    void AnalyzerInner();

    std::vector<std::shared_ptr<AstDeclaration>> m_decls;
};

#endif
