#ifndef AST_PARAMETER_HPP
#define AST_PARAMETER_HPP

#include <athens/ast/ast_declaration.hpp>

class AstParameter : public AstDeclaration {
public:
    AstParameter(const std::string &name, bool is_variadic,
        const SourceLocation &location);
    virtual ~AstParameter() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;

    bool IsVariadic() const { return m_is_variadic; }

private:
    bool m_is_variadic;
};

#endif
