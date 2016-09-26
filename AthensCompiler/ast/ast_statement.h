#ifndef AST_STATEMENT_H
#define AST_STATEMENT_H

#include "../source_location.h"

// Forward declaration
class AstVisitor;

class AstStatement {
    friend class AstIterator;
public:
    AstStatement(const SourceLocation &location);
    virtual ~AstStatement() = default;

    inline const SourceLocation &GetLocation() const { return m_location; }

    virtual void Visit(AstVisitor *visitor) = 0;

protected:
    SourceLocation m_location;
    AstStatement *m_next;
};

#endif