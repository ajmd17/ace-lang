#ifndef IDENTIFIER_TABLE_HPP
#define IDENTIFIER_TABLE_HPP

#include <athens/identifier.hpp>

#include <string>
#include <list>

class IdentifierTable {
public:
    IdentifierTable();
    IdentifierTable(const IdentifierTable &other);

    inline int GetIdentifierIndex() const { return m_identifier_index; }

    /** Constructs an identifier with the given name, as an alias to the given identifier. */
    Identifier *AddAlias(const std::string &name, const Identifier &aliasee);
    /** Constructs an identifier with the given name, and assigns an index to it. */
    Identifier *AddIdentifier(const std::string &name, int flags = 0);
    /** Adds the identifier to the table */
    Identifier *AddIdentifier(const Identifier &identifier);
    /** Look up an identifier by name. Returns nullptr if not found */
    Identifier *LookUpIdentifier(const std::string &name);

private:
    /** Uses std::list to avoid pointer invalidation. */
    std::list<Identifier> m_identifiers;
    /** To be incremented every time a new identifier is added */
    int m_identifier_index;
};

#endif
