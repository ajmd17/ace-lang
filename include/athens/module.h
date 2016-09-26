#ifndef MODULE_H
#define MODULE_H

#include <athens/scope.h>
#include <athens/source_location.h>

#include <list>
#include <string>

class Module {
public:
    Module(const std::string &name, const SourceLocation &location);
    Module(const Module &other);

    inline const std::string &GetName() const { return m_name; }
    inline const SourceLocation &GetLocation() const { return m_location; }

    Scope *OpenScope();
    Scope *CloseScope();

    /** use std::list to avoid pointer invalidation */
    std::list<Scope> m_scopes;
    /** the currently opened scope within m_scopes */
    Scope *m_current_scope;

private:
    std::string m_name;
    SourceLocation m_location;
};

#endif