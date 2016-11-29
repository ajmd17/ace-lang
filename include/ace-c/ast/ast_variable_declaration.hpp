#ifndef AST_VARIABLE_DECLARATION_HPP
#define AST_VARIABLE_DECLARATION_HPP

#include <ace-c/ast/ast_declaration.hpp>
#include <ace-c/ast/ast_expression.hpp>
#include <ace-c/ast/ast_type_specification.hpp>

#include <memory>

class AstVariableDeclaration : public AstDeclaration {
public:
    AstVariableDeclaration(const std::string &name,
        const std::shared_ptr<AstTypeSpecification> &type_specification,
        const std::shared_ptr<AstExpression> &assignment,
        const SourceLocation &location);
    virtual ~AstVariableDeclaration() = default;

    inline const std::shared_ptr<AstExpression> &GetAssignment() const
        { return m_assignment; }

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;

    bool m_assignment_already_visited;

protected:
    std::shared_ptr<AstTypeSpecification> m_type_specification;
    std::shared_ptr<AstExpression> m_assignment;
};

#endif
