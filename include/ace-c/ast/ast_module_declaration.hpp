#ifndef AST_MODULE_DECLARATION_HPP
#define AST_MODULE_DECLARATION_HPP

#include <ace-c/ast/ast_declaration.hpp>

class AstModuleDeclaration : public AstDeclaration {
public:
    AstModuleDeclaration(const std::string &name, const SourceLocation &location);

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
};

#endif
