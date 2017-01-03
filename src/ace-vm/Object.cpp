#include <ace-vm/Object.hpp>

#include <common/my_assert.hpp>

#include <cstring>
#include <cmath>

namespace ace {
namespace vm {

ObjectMap::ObjectBucket::ObjectBucket()
    : m_data(new Member*[DEFAULT_BUCKET_CAPACITY]),
      m_capacity(DEFAULT_BUCKET_CAPACITY),
      m_size(0)
{
}

ObjectMap::ObjectBucket::ObjectBucket(const ObjectBucket &other)
    : m_data(new Member*[other.m_capacity]),
      m_capacity(other.m_capacity),
      m_size(other.m_size)
{
    std::memcpy(m_data, other.m_data, sizeof(Member*) * other.m_size);
}

ObjectMap::ObjectBucket::~ObjectBucket()
{
    delete[] m_data;
}

void ObjectMap::ObjectBucket::Resize(size_t capacity)
{
    if (m_capacity < capacity) {
        Member **new_data = new Member*[capacity];
        std::memcpy(new_data, m_data, m_capacity);
        m_capacity = capacity;
        delete[] m_data;
        m_data = new_data;
    }
}

void ObjectMap::ObjectBucket::Push(Member *member)
{
    if (m_size == m_capacity) {
        Resize(m_capacity * 2);
    }
    m_data[m_size++] = member;
}

bool ObjectMap::ObjectBucket::Lookup(uint32_t hash, Member **out)
{
    for (size_t i = 0; i < m_size; i++) {
        if (m_data[i]->hash == hash) {
            *out = m_data[i];
            return true;
        }
    }
    return false;
}


ObjectMap::ObjectMap(size_t size)
    : m_size(size)
{
    ASSERT(m_size != 0);
    
    m_buckets = new ObjectMap::ObjectBucket[m_size];
}

ObjectMap::ObjectMap(const ObjectMap &other)
    : m_size(other.m_size)
{
    ASSERT(m_size != 0);

    m_buckets = new ObjectMap::ObjectBucket[m_size];
    for (size_t i = 0; i < m_size; i++) {
        m_buckets[i] = other.m_buckets[i];
    }
}

ObjectMap::~ObjectMap()
{
    delete[] m_buckets;
}

void ObjectMap::Push(uint32_t hash, Member *member)
{
    m_buckets[hash % m_size].Push(member);
}

Member *ObjectMap::Get(uint32_t hash)
{
    Member *res = nullptr;
    m_buckets[hash % m_size].Lookup(hash, &res);
    return res;
}

Object::Object(int size, uint32_t *hashes)
    : m_size(size)
{
    ASSERT(m_size != 0);

    m_members = new Member[m_size];
    m_object_map = new ObjectMap(m_size);

    for (int i = 0; i < m_size; i++) {
        m_members[i].hash = hashes[i];
        m_object_map->Push(hashes[i], &m_members[i]);
    }
}

Object::Object(const Object &other)
    : m_size(other.m_size)
{
    ASSERT(m_size != 0);

    m_members = new Member[m_size];
    m_object_map = new ObjectMap(m_size);

    // copy all members
    for (int i = 0; i < m_size; i++) {
        m_members[i] = other.m_members[i];
        m_object_map->Push(m_members[i].hash, &m_members[i]);
    }
}

Object::~Object()
{
    delete m_object_map;
    delete[] m_members;
}

} // namespace vm
} // namespace ace