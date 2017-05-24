#ifndef AST_IMPORT_HPP
#define AST_IMPORT_HPP

#include <ace-c/ast/AstStatement.hpp>
#include <ace-c/AstIterator.hpp>
#include <ace-c/Module.hpp>
#include <ace-c/CompilationUnit.hpp>

#include <utility>
#include <memory>

class AstImport : public AstStatement {
public:
    AstImport(const SourceLocation &location);
    virtual ~AstImport() = default;

    static void CopyModules(
        AstVisitor *visitor,
        std::shared_ptr<Module> &mod,
        bool check_lookup = false,
        bool update_tree_link = false);

    static bool TryOpenFile(
        const std::string &path,
        std::ifstream &is);

    virtual void Visit(AstVisitor *visitor, Module *mod) override = 0;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override = 0;
    virtual Pointer<AstStatement> Clone() const override = 0;

protected:
    /** The AST iterator that will be used by the imported module */
    AstIterator m_ast_iterator;

    void PerformImport(
        AstVisitor *visitor,
        Module *mod,
        const std::string &filepath);
};

#endif
