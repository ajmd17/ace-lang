#ifndef IDENTIFIER_HPP
#define IDENTIFIER_HPP

#include <athens/ast/ast_expression.hpp>
#include <athens/object_type.hpp>

#include <string>
#include <memory>

enum IdentifierFlags {
   FLAG_CONST = 0x01,
   FLAG_ALIAS = 0x02
};

class Identifier {
public:
    Identifier(const std::string &name, int index, int flags);
    Identifier(const Identifier &other);

    inline void IncUseCount() { m_usecount++; }

    inline const std::string &GetName() const { return m_name; }
    inline int GetIndex() const { return m_index; }
    inline int GetStackLocation() const { return m_stack_location; }
    inline void SetStackLocation(int stack_location) { m_stack_location = stack_location; }
    inline int GetUseCount() const { return m_usecount; }
    inline int GetFlags() const { return m_flags; }
    inline void SetFlags(int flags) { m_flags = flags; }
    inline std::weak_ptr<AstExpression> GetCurrentValue() const { return m_current_value; }
    inline void SetCurrentValue(const std::shared_ptr<AstExpression> &expr) { m_current_value = expr; }
    inline const ObjectType &GetObjectType() const { return m_object_type; }
    inline void SetObjectType(const ObjectType &object_type) { m_object_type = object_type; }
    inline bool TypeCompatible(const ObjectType &other_type) const
        { return ObjectType::TypeCompatible(m_object_type, other_type); }

private:
    std::string m_name;
    int m_index;
    int m_stack_location;
    int m_usecount;
    int m_flags;
    std::weak_ptr<AstExpression> m_current_value;
    ObjectType m_object_type;
};

#endif
