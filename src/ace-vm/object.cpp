#include <ace-vm/object.hpp>
#include <cstring>

namespace ace {
namespace vm {

Object::Object(int size, uint32_t *hashes)
    : m_size(size),
      m_members(new Member[size])
{
    for (int i = 0; i < m_size; i++) {
        m_members[i].hash = hashes[i];
    }
}

Object::Object(const Object &other)
    : m_size(other.m_size),
      m_members(new Member[other.m_size])
{
    // copy all members
    for (int i = 0; i < m_size; i++) {
        m_members[i] = other.m_members[i];
    }
}

Object::~Object()
{
    delete[] m_members;
}

Object &Object::operator=(const Object &other)
{
    if (m_size != other.m_size) {
        // size is not equal, so delete the members and resize
        if (m_members != nullptr) {
            delete[] m_members;
        }

        m_size = other.m_size;
        m_members = new Member[other.m_size];
    }

    // copy all members
    std::memcpy(m_members, other.m_members, sizeof(Member) * m_size);

    return *this;
}

Member *Object::LookupMemberFromHash(uint32_t hash) const
{
    // iterate through all members until
    // one with the hash has been found.

    for (int i = 0; i < m_size; i++) {
        if (m_members[i].hash == hash) {
            return &m_members[i];
        }
    }

    return nullptr;
}

} // namespace vm
} // namespace ace