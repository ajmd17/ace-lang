#ifndef AST_TRY_CATCH_HPP
#define AST_TRY_CATCH_HPP

#include <athens/ast/ast_statement.hpp>
#include <athens/ast/ast_expression.hpp>
#include <athens/ast/ast_block.hpp>

#include <memory>

class AstTryCatch : public AstStatement {
public:
    AstTryCatch(const std::shared_ptr<AstBlock> &try_block,
        const std::shared_ptr<AstBlock> &catch_block,
        const SourceLocation &location);
    virtual ~AstTryCatch() = default;

    virtual void Visit(AstVisitor *visitor) override;
    virtual void Build(AstVisitor *visitor) override;
    virtual void Optimize(AstVisitor *visitor) override;

private:
    std::shared_ptr<AstBlock> m_try_block;
    std::shared_ptr<AstBlock> m_catch_block;
};

#endif
