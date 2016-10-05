#ifndef AST_ITERATOR_H
#define AST_ITERATOR_H

#include <athens/ast/ast_statement.h>

#include <memory>
#include <vector>

class AstIterator {
public:
    AstIterator();
    AstIterator(const AstIterator &other);

    void Push(const std::shared_ptr<AstStatement> &statement);
    void ResetPosition();
    std::shared_ptr<AstStatement> Next();
    inline bool HasNext() const { return m_position < m_list.size(); }
    inline const SourceLocation &GetLocation() const { return m_list[m_position]->m_location; }

    std::vector<std::shared_ptr<AstStatement>> m_list;
    int m_position;
};

#endif