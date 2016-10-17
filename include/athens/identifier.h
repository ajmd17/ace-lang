#ifndef IDENTIFIER_H
#define IDENTIFIER_H

#include <athens/ast/ast_expression.h>

#include <string>
#include <memory>

enum IdentifierFlags {
   Flag_const = 0x01
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
    inline void SetCurrentValue(std::shared_ptr<AstExpression> expr) { m_current_value = expr; }

private:
    std::string m_name;
    int m_index;
    int m_stack_location;
    int m_usecount;
    int m_flags;
    std::weak_ptr<AstExpression> m_current_value;
};

#endif