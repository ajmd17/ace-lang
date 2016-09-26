#ifndef AST_MODULE_DECLARATION_H
#define AST_MODULE_DECLARATION_H

#include "ast_declaration.h"

class AstModuleDeclaration : public AstDeclaration {
public:
    AstModuleDeclaration(const std::string &name, const SourceLocation &location);

    virtual void Visit(AstVisitor *visitor);
};

#endif