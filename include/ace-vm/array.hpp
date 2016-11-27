#ifndef ARRAY_HPP
#define ARRAY_HPP

#include <ace-vm/stack_value.hpp>

class Array {
public:
    Array(int size = 0);
    Array(const Array &other);
    ~Array();

    Array &operator=(const Array &other);
    inline bool operator==(const Array &other) const { return this == &other; }

    inline int GetSize() const { return m_size; }
    inline StackValue &AtIndex(int index) { return m_buffer[index]; }
    inline const StackValue &AtIndex(int index) const { return m_buffer[index]; }

    void Push(const StackValue &value);
    void Pop();

private:
    int m_size;
    int m_capacity;
    StackValue *m_buffer;
};

#endif