#ifndef TYPE_INFO_HPP
#define TYPE_INFO_HPP

#include <common/my_assert.hpp>

namespace ace {
namespace vm {

class TypeInfo {
public:
    TypeInfo(const char *name, int size, char **names);
    TypeInfo(const TypeInfo &other);
    TypeInfo &operator=(const TypeInfo &other);
    ~TypeInfo();

    bool operator==(const TypeInfo &other) const;

    inline char *const GetName() const { return m_name; }
    inline int GetSize() const { return m_size; }
    inline char **const GetNames() const { return m_names; }
    inline const char *GetMemberName(int index) const
        { ASSERT(index < m_size); return m_names[index]; }

private:
    char *m_name;
    int m_size;
    char **m_names;
};

} // namespace vm
} // namespace ace

#endif