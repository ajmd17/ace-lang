#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <ace-vm/Value.hpp>

#include <cstdint>

#define DEFAULT_BUCKET_CAPACITY 4

namespace ace {
namespace vm {

struct Member {
    char *name;
    uint32_t hash;
    Value value;
};

class ObjectMap {
public:
    ObjectMap(size_t size);
    ObjectMap(const ObjectMap &other);
    ~ObjectMap();

    void Push(uint32_t hash, Member *member);
    Member *Get(uint32_t hash);

private:
    struct ObjectBucket {
        Member **m_data;
        size_t m_capacity;
        size_t m_size;

        ObjectBucket();
        ObjectBucket(const ObjectBucket &other);
        ~ObjectBucket();

        void Resize(size_t capacity);
        void Push(Member *member);
        bool Lookup(uint32_t hash, Member **out);
    };

    ObjectBucket *m_buckets;
    size_t m_size;
};

class Object {
public:
    Object(int size, char **names);
    Object(const Object &other);
    ~Object();

    inline bool operator==(const Object &other) const { return this == &other; }
    
    inline Member *LookupMemberFromHash(uint32_t hash) const { return m_object_map->Get(hash); }
    inline int GetSize() const { return m_size; }
    inline Member *GetMembers() const { return m_members; }
    inline Member &GetMember(int index) { return m_members[index]; }
    inline const Member &GetMember(int index) const { return m_members[index]; }

private:
    int m_size;
    Member *m_members;
    ObjectMap *m_object_map;
};

} // namespace vm
} // namespace ace

#endif
