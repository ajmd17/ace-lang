#ifndef MODULE_HPP
#define MODULE_HPP

#include <athens/scope.hpp>
#include <athens/source_location.hpp>
#include <athens/tree.hpp>

#include <list>
#include <string>

class Module {
public:
    Module(const std::string &name, const SourceLocation &location);
    Module(const Module &other) = delete;

    inline const std::string &GetName() const { return m_name; }
    inline const SourceLocation &GetLocation() const { return m_location; }

    /** Check to see if the identifier exists in multiple scopes, starting
        from the currently opened scope.
        If this_scope_only is set to true, only the current scope will be
        searched. */
    Identifier *LookUpIdentifier(const std::string &name, bool this_scope_only);

    Tree<Scope> m_scopes;

private:
    std::string m_name;
    SourceLocation m_location;
};

#endif
