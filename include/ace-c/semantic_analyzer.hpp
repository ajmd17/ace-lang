#ifndef SEMANTIC_ANALYZER_HPP
#define SEMANTIC_ANALYZER_HPP

#include <ace-c/ast_visitor.hpp>
#include <ace-c/ast/ast_declaration.hpp>
#include <ace-c/identifier.hpp>
#include <ace-c/enums.hpp>

#include <memory>
#include <vector>
#include <string>

// forward declaration
class Module;

struct IdentifierLookupResult {
    // union {
    Identifier *as_identifier = nullptr;
    Module *as_module = nullptr;
    ObjectType as_type;
    // };

    IdentifierType type;
};

class SemanticAnalyzer : public AstVisitor {
public:
    static IdentifierLookupResult LookupIdentifier(AstVisitor *visitor, Module *mod, const std::string &name);

public:
    SemanticAnalyzer(AstIterator *ast_iterator, CompilationUnit *compilation_unit);
    SemanticAnalyzer(const SemanticAnalyzer &other);

    /** Generates the compilation unit structure from the given statement iterator */
    void Analyze(bool expect_module_decl = true);

private:
    void AnalyzerInner();
};

#endif
