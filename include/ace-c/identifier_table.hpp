#ifndef IDENTIFIER_TABLE_HPP
#define IDENTIFIER_TABLE_HPP

#include <ace-c/identifier.hpp>

#include <string>
#include <memory>
#include <vector>

class IdentifierTable {
public:
    IdentifierTable();
    IdentifierTable(const IdentifierTable &other);

    int CountUsedVariables() const;
    inline void PopIdentifier() { m_identifiers.pop_back(); m_identifier_index--; }
    inline int GetIdentifierIndex() const { return m_identifier_index; }
    inline std::vector<std::shared_ptr<Identifier>> &GetIdentifiers()
        { return m_identifiers; }
    inline const std::vector<std::shared_ptr<Identifier>> &GetIdentifiers() const
        { return m_identifiers; }

    /** Constructs an identifier with the given name, as an alias to the given identifier. */
    Identifier *AddAlias(const std::string &name, Identifier *aliasee);
    /** Constructs an identifier with the given name, and assigns an index to it. */
    Identifier *AddIdentifier(const std::string &name, int flags = 0);
    /** Look up an identifier by name. Returns nullptr if not found */
    Identifier *LookUpIdentifier(const std::string &name);

private:
    /** To be incremented every time a new identifier is added */
    int m_identifier_index;
    /** List of all identifiers in the table */
    std::vector<std::shared_ptr<Identifier>> m_identifiers;
};

#endif
