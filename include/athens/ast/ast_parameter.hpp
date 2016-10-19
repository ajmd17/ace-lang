#ifndef AST_PARAMETER_HPP
#define AST_PARAMETER_HPP

#include <athens/ast/ast_declaration.hpp>

class AstParameter : public AstDeclaration {
public:
    AstParameter(const std::string &name, const SourceLocation &location);
    virtual ~AstParameter() = default;

    virtual void Visit(AstVisitor *visitor) override;
    virtual void Build(AstVisitor *visitor) override;
    virtual void Optimize(AstVisitor *visitor) override;
};

#endif
