#ifndef SCOPE_HPP
#define SCOPE_HPP

#include <ace-c/IdentifierTable.hpp>
#include <ace-c/SymbolType.hpp>
#include <ace-c/SourceLocation.hpp>

#include <vector>
#include <utility>

typedef std::pair<SymbolTypePtr_t, SourceLocation> ReturnType_t;

enum ScopeType {
    SCOPE_TYPE_NORMAL,
    SCOPE_TYPE_FUNCTION,
    SCOPE_TYPE_PURE_FUNCTION,
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

    inline void AddReturnType(const SymbolTypePtr_t &type, const SourceLocation &location) 
        { m_return_types.push_back({type, location}); }
    inline const std::vector<ReturnType_t> &GetReturnTypes() const { return m_return_types; }

private:
    IdentifierTable m_identifier_table;
    ScopeType m_scope_type;
    std::vector<ReturnType_t> m_return_types;
};

#endif
