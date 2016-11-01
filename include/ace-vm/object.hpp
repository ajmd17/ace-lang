#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <ace-vm/stack_value.hpp>

class Object {
public:
    Object(int size);
    Object(const Object &other);
    ~Object();

    Object &operator=(const Object &other);
    inline bool operator==(const Object &other) const { return this == &other; }

    inline int GetSize() const { return m_size; }
    inline StackValue &GetMember(int index) { return m_members[index]; }
    inline const StackValue &GetMember(int index) const { return m_members[index]; }

private:
    int m_size;
    StackValue *m_members;
};

#endif
