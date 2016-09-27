#ifndef MODULE_H
#define MODULE_H

#include <athens/scope.h>
#include <athens/source_location.h>
#include <athens/tree.h>

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
    const Identifier *LookUpIdentifier(const std::string &name, bool this_scope_only) const;

    Tree<Scope> m_scopes;

private:
    std::string m_name;
    SourceLocation m_location;
};

#endif