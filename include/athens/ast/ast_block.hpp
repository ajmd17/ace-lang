#ifndef AST_BLOCK_HPP
#define AST_BLOCK_HPP

#include <athens/ast/ast_statement.hpp>

#include <vector>
#include <memory>

class AstBlock : public AstStatement {
public:
    AstBlock(const SourceLocation &location);

    inline void AddChild(const std::shared_ptr<AstStatement> &stmt) { m_children.push_back(stmt); }
    inline int NumLocals() const { return m_num_locals; }

    virtual void Visit(AstVisitor *visitor) override;
    virtual void Build(AstVisitor *visitor) override;
    virtual void Optimize(AstVisitor *visitor) override;

protected:
    std::vector<std::shared_ptr<AstStatement>> m_children;
    int m_num_locals;
};

#endif
