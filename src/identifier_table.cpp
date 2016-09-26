#include <athens/identifier_table.h>

IdentifierTable::IdentifierTable()
{
}

IdentifierTable::IdentifierTable(const IdentifierTable &other)
    : m_identifiers(other.m_identifiers)
{
}

void IdentifierTable::AddAlias(const std::string &name, const Identifier &aliasee)
{
    int index = aliasee.GetIndex();
    Identifier ident(name, index);
    m_identifiers.push_back(ident);
}

void IdentifierTable::AddIdentifier(const std::string &name)
{
    int index = static_cast<int>(m_identifiers.size());
    Identifier ident(name, index);
    m_identifiers.push_back(ident);
}

void IdentifierTable::AddIdentifier(const Identifier &identifier)
{
    m_identifiers.push_back(identifier);
}

/** Look up an identifier by name. Returns nullptr if not found */
const Identifier *IdentifierTable::LookUpIdentifier(const std::string &name) const
{
    for (const Identifier &ident : m_identifiers) {
        if (ident.GetName() == name) {
            return &ident;
        }
    }
    return nullptr;
}