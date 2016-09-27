#ifndef AST_VARIABLE_DECLARATION_H
#define AST_VARIABLE_DECLARATION_H

#include <athens/ast/ast_declaration.h>
#include <athens/ast/ast_expression.h>

#include <memory>

class AstVariableDeclaration : public AstDeclaration {
public:
    AstVariableDeclaration(const std::string &name, 
        std::unique_ptr<AstExpression> &&assignment,
        const SourceLocation &location);
    virtual ~AstVariableDeclaration() = default;

    virtual void Visit(AstVisitor *visitor);

protected:
    std::unique_ptr<AstExpression> m_assignment;
};

#endif