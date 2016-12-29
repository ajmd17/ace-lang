#ifndef TYPE_INFO_HPP
#define TYPE_INFO_HPP

#include <common/my_assert.hpp>

namespace ace {
namespace vm {

class TypeInfo {
public:
    TypeInfo(int size, uint32_t *hashes);
    TypeInfo(const TypeInfo &other);
    TypeInfo &operator=(const TypeInfo &other);
    ~TypeInfo();

    bool operator==(const TypeInfo &other) const;

    inline int GetSize() const { return m_size; }
    inline uint32_t *GetHashes() const { return m_hashes; }
    inline uint32_t GetMemberHash(int index) const
        { ASSERT(index < m_size); return m_hashes[index]; }

private:
    int m_size;
    uint32_t *m_hashes;
};

} // namespace vm
} // namespace ace

#endif