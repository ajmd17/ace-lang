#ifndef STATIC_MEMORY_HPP
#define STATIC_MEMORY_HPP

#include <ace-vm/stack_value.hpp>

#include <cassert>

class StaticMemory {
public:
    static const uint16_t static_size;

public:
    StaticMemory();
    StaticMemory(const StaticMemory &other) = delete;
    ~StaticMemory();

    inline StackValue &operator[](size_t index)
    {
        assert(index < static_size && "out of bounds");
        return m_data[index];
    }

    inline const StackValue &operator[](size_t index) const
    {
        assert(index < static_size && "out of bounds");
        return m_data[index];
    }

    // push a value to the stack
    inline void Store(const StackValue &value)
    {
        assert(m_sp < static_size && "not enough static memory");
        m_data[m_sp++] = value;
    }

private:
    StackValue *m_data;
    size_t m_sp;
};

#endif
