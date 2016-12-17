#ifndef STATIC_MEMORY_HPP
#define STATIC_MEMORY_HPP

#include <ace-vm/stack_value.hpp>
#include <common/my_assert.hpp>

#include <utility>

namespace ace {
namespace vm {

class StaticMemory {
public:
    static const uint16_t static_size;

public:
    StaticMemory();
    StaticMemory(const StaticMemory &other) = delete;
    ~StaticMemory();

    /** Delete everything in static memory */
    void Purge();

    inline Value &operator[](size_t index)
    {
        ASSERT_MSG(index < static_size, "out of bounds");
        return m_data[index];
    }

    inline const Value &operator[](size_t index) const
    {
        ASSERT_MSG(index < static_size, "out of bounds");
        return m_data[index];
    }

    // move a value to static memory
    inline void Store(Value &&value)
    {
        ASSERT_MSG(m_sp < static_size, "not enough static memory");
        m_data[m_sp++] = std::move(value);
    }

private:
    Value *m_data;
    size_t m_sp;
};

} // namespace vm
} // namespace ace

#endif
