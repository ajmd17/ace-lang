#ifndef SCOPE_HPP
#define SCOPE_HPP

#include <ace-c/identifier_table.hpp>
#include <ace-c/object_type.hpp>
#include <ace-c/source_location.hpp>

#include <vector>
#include <utility>

enum ScopeType {
    SCOPE_TYPE_NORMAL,
    SCOPE_TYPE_FUNCTION,
    SCOPE_TYPE_LOOP,
};

class Scope {
    friend class Module;
public:
    Scope();
    Scope(ScopeType scope_type);
    Scope(const Scope &other);

    inline IdentifierTable &GetIdentifierTable() { return m_identifier_table; }
    inline const IdentifierTable &GetIdentifierTable() const { return m_identifier_table; }
    inline ScopeType GetScopeType() const { return m_scope_type; }
    inline void AddReturnType(const ObjectType &type, const SourceLocation &location)
        { m_return_types.push_back({type, location}); }
    inline const std::vector<std::pair<ObjectType, SourceLocation>> &GetReturnTypes() const
        { return m_return_types; }

private:
    IdentifierTable m_identifier_table;
    ScopeType m_scope_type;
    std::vector<std::pair<ObjectType, SourceLocation>> m_return_types;
};

#endif
