#ifndef IDENTIFIER_H
#define IDENTIFIER_H

#include <string>

enum IdentifierFlags {
   Flag_const = 0x00,
   Flag_literal = 0x01,
};

class Identifier {
public:
    Identifier(const std::string &name, int index);
    Identifier(const Identifier &other);

    inline void IncUseCount() { m_usecount++; }

    inline const std::string &GetName() const { return m_name; }
    inline int GetIndex() const { return m_index; }
    inline int GetUseCount() const { return m_usecount; }
    inline int GetFlags() const { return m_flags; }
    inline void SetFlags(int flags) { m_flags = flags; }

private:
    std::string m_name;
    int m_index;
    int m_usecount;
    int m_flags;
};

#endif