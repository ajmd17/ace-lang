#include <ace-c/identifier_table.hpp>
#include <ace-c/configuration.hpp>

#include <unordered_set>
#include <common/my_assert.hpp>

IdentifierTable::IdentifierTable()
    : m_identifier_index(0)
{
}

IdentifierTable::IdentifierTable(const IdentifierTable &other)
    : m_identifier_index(other.m_identifier_index),
      m_identifiers(other.m_identifiers)
{
}

int IdentifierTable::CountUsedVariables() const
{
    std::unordered_set<int> used_variables;
    for (auto &ident : m_identifiers) {
        if (!ace::compiler::Config::cull_unused_objects || ident->GetUseCount() > 0) {
            if (used_variables.find(ident->GetIndex()) == used_variables.end()) {
                used_variables.insert(ident->GetIndex());
            }
        }
    }
    return used_variables.size();
}

Identifier *IdentifierTable::AddAlias(const std::string &name, Identifier *aliasee)
{
    ASSERT(aliasee != nullptr);
    m_identifiers.push_back(std::shared_ptr<Identifier>(new Identifier(name,
        aliasee->GetIndex(), aliasee->GetFlags() | FLAG_ALIAS)));
    return m_identifiers.back().get();
}

Identifier *IdentifierTable::AddIdentifier(const std::string &name, int flags)
{
    m_identifiers.push_back(std::shared_ptr<Identifier>(new Identifier(name,
        m_identifier_index++, flags)));
    return m_identifiers.back().get();
}

Identifier *IdentifierTable::LookUpIdentifier(const std::string &name)
{
    for (auto &ident : m_identifiers) {
        if (ident->GetName() == name) {
            return ident.get();
        }
    }
    return nullptr;
}
