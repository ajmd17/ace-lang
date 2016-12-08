#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <ace-vm/stack_value.hpp>

#include <cstdint>

struct Member {
    uint32_t hash;
    StackValue value;
};

class Object {
public:
    Object(int size, uint32_t *hashes);
    Object(const Object &other);
    Object &operator=(const Object &other);
    ~Object();

    Member *LookupMemberFromHash(uint32_t hash) const;

    inline bool operator==(const Object &other) const { return this == &other; }

    inline int GetSize() const { return m_size; }
    inline Member &GetMember(int index) { return m_members[index]; }
    inline const Member &GetMember(int index) const { return m_members[index]; }

private:
    int m_size;
    Member *m_members;
};

#endif
