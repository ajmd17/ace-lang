#ifndef AST_TRY_CATCH_HPP
#define AST_TRY_CATCH_HPP

#include <ace-c/ast/ast_statement.hpp>
#include <ace-c/ast/ast_expression.hpp>
#include <ace-c/ast/ast_block.hpp>

#include <memory>

class AstTryCatch : public AstStatement {
public:
    AstTryCatch(const std::shared_ptr<AstBlock> &try_block,
        const std::shared_ptr<AstBlock> &catch_block,
        const SourceLocation &location);
    virtual ~AstTryCatch() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual void Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    virtual void Recreate(std::ostringstream &ss) override;

private:
    std::shared_ptr<AstBlock> m_try_block;
    std::shared_ptr<AstBlock> m_catch_block;
};

#endif
