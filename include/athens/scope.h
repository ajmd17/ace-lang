#ifndef SCOPE_H
#define SCOPE_H

#include <athens/identifier_table.h>

#include <list>

class Scope {
    friend class Module;
public:
    Scope();
    Scope(const Scope &other);

    inline IdentifierTable &GetIdentifierTable() { return m_identifier_table; }
    inline const IdentifierTable &GetIdentifierTable() const { return m_identifier_table; }

private:
    IdentifierTable m_identifier_table;
    Scope *m_parent;

    /** Use std::list to avoid pointer invalidation */
    std::list<Scope> m_inner_scopes;
};

#endif