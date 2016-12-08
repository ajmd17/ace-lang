#include <ace-vm/type_info.hpp>
#include <cstring>

TypeInfo::TypeInfo(int size, uint32_t *hashes)
    : m_size(size), m_hashes(new uint32_t[size])
{
    // copy all hashes
    std::memcpy(m_hashes, hashes, sizeof(uint32_t) * m_size);
}

TypeInfo::TypeInfo(const TypeInfo &other)
    : m_size(other.m_size), m_hashes(new uint32_t[other.m_size])
{
    // copy all hashes
    std::memcpy(m_hashes, other.m_hashes, sizeof(uint32_t) * m_size);
}

TypeInfo &TypeInfo::operator=(const TypeInfo &other)
{
    if (m_size != other.m_size) {
        delete[] m_hashes;
        m_hashes = new uint32_t[other.m_size];
    }

    m_size = other.m_size;

    // copy all hashes
    std::memcpy(m_hashes, other.m_hashes, sizeof(uint32_t) * m_size);

    return *this;
}

TypeInfo::~TypeInfo()
{
    delete[] m_hashes;
}

bool TypeInfo::operator==(const TypeInfo &other) const
{
    if (m_size != other.m_size) {
        return false;
    }

    // compare hashes
    for (int i = 0; i < m_size; i++) {
        if (m_hashes[i] != other.m_hashes[i]) {
            return false;
        }
    }

    return true;
}