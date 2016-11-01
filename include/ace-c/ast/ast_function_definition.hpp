#ifndef AST_FUNCTION_DEFINITION_HPP
#define AST_FUNCTION_DEFINITION_HPP

#include <ace-c/ast/ast_declaration.hpp>
#include <ace-c/ast/ast_function_expression.hpp>

#include <memory>
#include <vector>

class AstFunctionDefinition : public AstDeclaration {
public:
    AstFunctionDefinition(const std::string &name,
        const std::shared_ptr<AstFunctionExpression> &expr,
        const SourceLocation &location);
    virtual ~AstFunctionDefinition() = default;

    inline const std::shared_ptr<AstFunctionExpression> &GetAssignment() const
        { return m_expr; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;

protected:
    std::shared_ptr<AstFunctionExpression> m_expr;
};

#endif
