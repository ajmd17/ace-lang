#ifndef AST_META_BLOCK_HPP
#define AST_META_BLOCK_HPP

#include <ace-c/ast/AstStatement.hpp>
#include <ace-c/ast/AstBlock.hpp>
#include <ace-c/ast/AstFunctionExpression.hpp>

#include <memory>
#include <vector>

class AstMetaBlock : public AstStatement {
public:
    AstMetaBlock(const std::shared_ptr<AstBlock> &block,
        const SourceLocation &location);
    virtual ~AstMetaBlock() = default;

    virtual void Visit(AstVisitor *visitor, Module *mod) override;
    virtual std::unique_ptr<Buildable> Build(AstVisitor *visitor, Module *mod) override;
    virtual void Optimize(AstVisitor *visitor, Module *mod) override;
    
    virtual Pointer<AstStatement> Clone() const override;

protected:
    std::shared_ptr<AstBlock> m_block;

    // set while analyzing
    std::shared_ptr<AstFunctionExpression> m_result_closure;

    inline Pointer<AstMetaBlock> CloneImpl() const
    {
        return Pointer<AstMetaBlock>(new AstMetaBlock(
            CloneAstNode(m_block),
            m_location
        ));
    }
};

#endif
