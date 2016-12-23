#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <ace-vm/value.hpp>

#include <cstdint>

namespace ace {
namespace vm {

struct Member {
    uint32_t hash;
    Value value;
};

class Object {
public:
    Object(int size, uint32_t *hashes);
    Object(const Object &other);
    ~Object();

    Object &operator=(const Object &other);
    inline bool operator==(const Object &other) const { return this == &other; }

    Member *LookupMemberFromHash(uint32_t hash) const;

    inline int GetSize() const { return m_size; }
    inline Member &GetMember(int index) { return m_members[index]; }
    inline const Member &GetMember(int index) const { return m_members[index]; }

private:
    int m_size;
    Member *m_members;
};

} // namespace vm
} // namespace ace

#endif
