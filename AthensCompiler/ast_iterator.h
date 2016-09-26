#ifndef AST_ITERATOR_H
#define AST_ITERATOR_H

#include "ast/ast_statement.h"

class AstIterator {
public:
    AstIterator();
    AstIterator(const AstIterator &other);

    void PushBack(AstStatement *statement);
    void ResetPosition();
    AstStatement *Next();
    inline bool HasNext() const { return m_current != nullptr; }
    inline const SourceLocation &GetLocation() const { return m_current->m_location; }

private:
    AstStatement *m_start;
    AstStatement *m_current;
    AstStatement *m_top;
};

#endif