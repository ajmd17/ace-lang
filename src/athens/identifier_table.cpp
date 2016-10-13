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

Identifier *IdentifierTable::AddAlias(const std::string &name, const Identifier &aliasee)
{
    m_identifiers.push_back(Identifier(name, 
        aliasee.GetIndex(), aliasee.GetFlags()));
    return &m_identifiers.back();
}

Identifier *IdentifierTable::AddIdentifier(const std::string &name, int flags)
{
    m_identifiers.push_back(Identifier(name, 
        m_identifier_index++, flags));
    return &m_identifiers.back();
}

Identifier *IdentifierTable::AddIdentifier(const Identifier &identifier)
{
    m_identifiers.push_back(identifier);
    return &m_identifiers.back();
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