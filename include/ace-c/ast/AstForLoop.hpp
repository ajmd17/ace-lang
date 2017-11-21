#ifndef AST_FOR_LOOP_HPP
#define AST_FOR_LOOP_HPP

#include <ace-c/ast/AstStatement.hpp>
#include <ace-c/ast/AstActionExpression.hpp>
#include <ace-c/ast/AstParameter.hpp>
#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/ast/AstBlock.hpp>
#include <ace-c/ast/AstFunctionExpression.hpp>

#include <memory>
#include <vector>

class AstForLoop : public AstStatement {
public:
    AstForLoop(const std::vector<std::shared_ptr<AstParameter>> &params,
        const std::shared_ptr<AstExpression> &iteree,
        const std::shared_ptr<AstBlock> &block,
        const SourceLocation &location);
    virtual ~AstForLoop() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    
    virtual Pointer<AstStatement> Clone() const override;

private:
    std::vector<std::shared_ptr<AstParameter>> m_params;
    std::shared_ptr<AstExpression> m_iteree;
    std::shared_ptr<AstBlock> m_block;
    int m_num_locals;

    std::shared_ptr<AstExpression> m_expr;

    inline Pointer<AstForLoop> CloneImpl() const
    {
        return Pointer<AstForLoop>(new AstForLoop(
            CloneAllAstNodes(m_params),
            CloneAstNode(m_iteree),
            CloneAstNode(m_block),
            m_location
        ));
    }
};

#endif
