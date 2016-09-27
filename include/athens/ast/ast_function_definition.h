#ifndef AST_FUNCTION_DEFINITION_H
#define AST_FUNCTION_DEFINITION_H

#include <athens/ast/ast_declaration.h>
#include <athens/ast/ast_parameter.h>
#include <athens/ast/ast_block.h>

#include <memory>
#include <vector>

class AstFunctionDefinition : public AstDeclaration {
public:
    AstFunctionDefinition(const std::string &name,
        std::vector<std::unique_ptr<AstParameter>> &&parameters,
        std::unique_ptr<AstBlock> &&block,
        const SourceLocation &location);
    virtual ~AstFunctionDefinition() = default;

    virtual void Visit(AstVisitor *visitor);

protected:
    std::vector<std::unique_ptr<AstParameter>> m_parameters;
    std::unique_ptr<AstBlock> m_block;
};

#endif