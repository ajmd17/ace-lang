#ifndef MODULE_HPP
#define MODULE_HPP

#include <ace-c/scope.hpp>
#include <ace-c/source_location.hpp>
#include <ace-c/tree.hpp>
#include <ace-c/object_type.hpp>

#include <vector>
#include <string>
#include <memory>

class Module {
public:
    Module(const std::string &name, const SourceLocation &location);
    Module(const Module &other) = delete;

    inline const std::string &GetName() const { return m_name; }
    inline const SourceLocation &GetLocation() const { return m_location; }

    /** Check to see if the identifier exists in multiple scopes, starting
        from the currently opened scope.
        If this_scope_only is set to true, only the current scope will be
        searched.
    */
    Identifier *LookUpIdentifier(const std::string &name, bool this_scope_only);
    /** Check to see if the identifier exists in this scope or above this one.
        Will only search the number of depth levels it is given.
        Pass `1` for this scope only.
    */
    Identifier *LookUpIdentifierDepth(const std::string &name, int depth_level);

    /** Check to see if a user has declared a type with this name */
    bool LookUpUserType(const std::string &type, ObjectType &out);
    /** Add this type to the list of user-defined types */
    void AddUserType(const ObjectType &type);

    Tree<Scope> m_scopes;

private:
    std::string m_name;
    SourceLocation m_location;
    std::vector<ObjectType> m_user_types;
};

#endif
