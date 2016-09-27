#include <athens/identifier_table.h>

IdentifierTable::IdentifierTable()
    : m_identifier_index(0)
{
}

IdentifierTable::IdentifierTable(const IdentifierTable &other)
    : m_identifier_index(other.m_identifier_index),
      m_identifiers(other.m_identifiers)
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
    Identifier ident(name, m_identifier_index++);
    m_identifiers.push_back(ident);
}

void IdentifierTable::AddIdentifier(const Identifier &identifier)
{
    m_identifiers.push_back(identifier);
}

Identifier *IdentifierTable::LookUpIdentifier(const std::string &name)
{
    for (Identifier &ident : m_identifiers) {
        if (ident.GetName() == name) {
            return &ident;
        }
    }
    return nullptr;
}