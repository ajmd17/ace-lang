#ifndef AST_PARAMETER_H
#define AST_PARAMETER_H

#include <athens/ast/ast_declaration.h>

class AstParameter : public AstDeclaration {
public:
    AstParameter(const std::string &name, const SourceLocation &location);
    virtual ~AstParameter() = default;

    virtual void Visit(AstVisitor *visitor);
};

#endif