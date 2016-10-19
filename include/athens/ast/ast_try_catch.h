#ifndef AST_TRY_CATCH_H
#define AST_TRY_CATCH_H

#include <athens/ast/ast_statement.h>
#include <athens/ast/ast_expression.h>
#include <athens/ast/ast_block.h>

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
