#include <ace-vm/Object.hpp>

#include <common/my_assert.hpp>
#include <common/hasher.hpp>

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

Object::Object(TypeInfo *type_ptr, const Value &type_ptr_value)
    : m_type_ptr(type_ptr),
      m_type_ptr_value(type_ptr_value)
{
    ASSERT(m_type_ptr != nullptr);

    size_t size = m_type_ptr->GetSize();
    ASSERT(size > 0);

    auto **names = m_type_ptr->GetNames();
    ASSERT(names != nullptr);

    m_members = new Member[size];
    m_object_map = new ObjectMap(size);

    for (size_t i = 0; i < size; i++) {
        // compute hash for member name
        uint32_t hash = hash_fnv_1(names[i]);
        m_members[i].hash = hash;
        
        m_object_map->Push(hash, &m_members[i]);
    }
}

Object::Object(const Object &other)
    : m_type_ptr(other.m_type_ptr),
      m_type_ptr_value(other.m_type_ptr_value)
{
    ASSERT(m_type_ptr != nullptr);

    size_t size = m_type_ptr->GetSize();
    ASSERT(size > 0);

    auto **names = m_type_ptr->GetNames();
    ASSERT(names != nullptr);

    m_members = new Member[size];
    m_object_map = new ObjectMap(size);

    // copy all members
    for (size_t i = 0; i < size; i++) {
        m_members[i] = other.m_members[i];
        // compute hash for member name
        uint32_t hash = hash_fnv_1(names[i]);
        m_members[i].hash = hash;
        
        m_object_map->Push(hash, &m_members[i]);
    }
}

Object::~Object()
{
    delete m_object_map;
    delete[] m_members;
}

void Object::GetRepresentation(utf::Utf8String &out_str, bool add_type_name) const
{
    // get type
    ASSERT(m_type_ptr != nullptr);

    size_t size = m_type_ptr->GetSize();
    ASSERT(size > 0);

    /*if (add_type_name)
        // add the type name
        out += m
    }*/

    out_str += "{";

    for (size_t i = 0; i < size; i++) {
        vm::Member &mem = m_members[i];

        out_str += "\"";
        out_str += m_type_ptr->GetMemberName(i);
        out_str += "\":";

        mem.value.ToRepresentation(out_str, add_type_name);

        if (i != size - 1) {
            out_str += ",";
        }
    }

    out_str += "}";
}

} // namespace vm
} // namespace ace