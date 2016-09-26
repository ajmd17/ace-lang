#ifndef IDENTIFIER_TABLE_H
#define IDENTIFIER_TABLE_H

#include "identifier.h"

#include <string>
#include <list>

class IdentifierTable {
public:
    IdentifierTable();
    IdentifierTable(const IdentifierTable &other);

    /** Constructs an identifier with the given name, as an alias to the given identifier. */
    void AddAlias(const std::string &name, const Identifier &aliasee);
    /** Constructs an identifier with the given name, and assigns an index to it. */
    void AddIdentifier(const std::string &name);
    /** Adds the identifier to the table */
    void AddIdentifier(const Identifier &identifier);
    /** Look up an identifier by name. Returns nullptr if not found */
    const Identifier *LookUpIdentifier(const std::string &name) const;

private:
    /** Uses std::list to avoid pointer invalidation. */
    std::list<Identifier> m_identifiers;
};

#endif