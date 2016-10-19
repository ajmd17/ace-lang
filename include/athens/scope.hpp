#ifndef SCOPE_HPP
#define SCOPE_HPP

#include <athens/identifier_table.hpp>

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
};

#endif
