#ifndef AST_BLOCK_H
#define AST_BLOCK_H

#include <athens/ast/ast_statement.h>

#include <vector>
#include <memory>

class AstBlock : public AstStatement {
public:
    AstBlock(const SourceLocation &location);

    inline void AddChild(std::unique_ptr<AstStatement> &stmt) { m_children.push_back(std::move(stmt)); }

    virtual void Visit(AstVisitor *visitor);

protected:
    std::vector<std::unique_ptr<AstStatement>> m_children;
};

#endif