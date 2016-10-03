#ifndef AST_STATEMENT_H
#define AST_STATEMENT_H

#include <athens/source_location.h>

#include <memory>

// Forward declaration
class AstVisitor;

class AstStatement {
    friend class AstIterator;
public:
    AstStatement(const SourceLocation &location);
    virtual ~AstStatement() = default;

    inline const SourceLocation &GetLocation() const { return m_location; }

    virtual void Visit(AstVisitor *visitor) = 0;
    virtual void Build(AstVisitor *visitor) const = 0;
    virtual void Optimize(AstVisitor *visitor) = 0;

protected:
    SourceLocation m_location;
};

#endif