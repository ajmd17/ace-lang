#ifndef SEMANTIC_ANALYZER_HPP
#define SEMANTIC_ANALYZER_HPP

#include <ace-c/AstVisitor.hpp>
#include <ace-c/Identifier.hpp>
#include <ace-c/enums.hpp>
#include <ace-c/ast/AstArgument.hpp>

#include <memory>
#include <vector>
#include <string>
#include <utility>

// forward declaration
class Module;

struct IdentifierLookupResult {
    // union {
    Identifier *as_identifier = nullptr;
    Module *as_module = nullptr;
    SymbolTypePtr_t as_type;
    // };

    IdentifierType type;
};

class SemanticAnalyzer : public AstVisitor {
public:
    static IdentifierLookupResult LookupIdentifier(AstVisitor *visitor, Module *mod, const std::string &name);

    static std::pair<SymbolTypePtr_t, std::vector<std::shared_ptr<AstArgument>>>
    SubstituteFunctionArgs(
        AstVisitor *visitor,
        Module *mod, 
        const SymbolTypePtr_t &identifier_type, 
        const std::vector<std::shared_ptr<AstArgument>> &args,
        const SourceLocation &location);

public:
    SemanticAnalyzer(AstIterator *ast_iterator, CompilationUnit *compilation_unit);
    SemanticAnalyzer(const SemanticAnalyzer &other);

    /** Generates the compilation unit structure from the given statement iterator */
    void Analyze(bool expect_module_decl = true);

private:
    void AnalyzerInner();
};

#endif
