#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <ace-vm/stack_value.hpp>

struct Member {
    Member() {}
    Member(const StackValue &name, const StackValue &value)
        : name(name), value(value) {}
    Member(const Member &other)
        : name(other.name), value(other.value) {}

    inline StackValue &GetName() { return name; }
    inline const StackValue &GetName() const { return name; }
    inline StackValue &GetValue() { return value; }
    inline const StackValue &GetValue() const { return value; }

    StackValue name;
    StackValue value;
};

class Object {
public:
    Object(int size);
    Object(const Object &other);
    ~Object();

    Object &operator=(const Object &other);
    inline bool operator==(const Object &other) const { return this == &other; }

    inline int GetSize() const { return m_size; }
    inline Member &GetMember(int index) { return m_members[index]; }
    inline const Member &GetMember(int index) const { return m_members[index]; }

private:
    int m_size;
    Member *m_members;
};

#endif
