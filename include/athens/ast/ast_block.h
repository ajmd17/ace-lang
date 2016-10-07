#ifndef AST_BLOCK_H
#define AST_BLOCK_H

#include <athens/ast/ast_statement.h>

#include <vector>
#include <memory>

class AstBlock : public AstStatement {
public:
    AstBlock(const SourceLocation &location);

    inline void AddChild(const std::shared_ptr<AstStatement> &stmt) { m_children.push_back(stmt); }

    virtual void Visit(AstVisitor *visitor);
    virtual void Build(AstVisitor *visitor) const;
    virtual void Optimize(AstVisitor *visitor);

protected:
    std::vector<std::shared_ptr<AstStatement>> m_children;
    int m_num_locals;
};

#endif