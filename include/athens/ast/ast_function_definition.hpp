#ifndef AST_FUNCTION_DEFINITION_HPP
#define AST_FUNCTION_DEFINITION_HPP

#include <athens/ast/ast_declaration.hpp>
#include <athens/ast/ast_parameter.hpp>
#include <athens/ast/ast_block.hpp>

#include <memory>
#include <vector>

class AstFunctionDefinition : public AstDeclaration {
public:
    AstFunctionDefinition(const std::string &name,
        const std::vector<std::shared_ptr<AstParameter>> &parameters,
        const std::shared_ptr<AstBlock> &block,
        const SourceLocation &location);
    virtual ~AstFunctionDefinition() = default;

    virtual void Visit(AstVisitor *visitor) override;
    virtual void Build(AstVisitor *visitor) override;
    virtual void Optimize(AstVisitor *visitor) override;

protected:
    std::vector<std::shared_ptr<AstParameter>> m_parameters;
    std::shared_ptr<AstBlock> m_block;

    // set while compiling
    int m_static_id;
};

#endif
