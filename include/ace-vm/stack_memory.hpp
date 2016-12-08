#ifndef STACK_MEMORY_HPP
#define STACK_MEMORY_HPP

#include <ace-vm/stack_value.hpp>
#include <common/my_assert.hpp>

#include <array>

class Stack {
public:
    static const uint16_t stack_size;

public:
    Stack();
    Stack(const Stack &other) = delete;
    ~Stack();

    /** Purge all items on the stack */
    void Purge();
    /** Mark all items on the stack to not be garbage collected */
    void MarkAll();

    inline StackValue *GetData() { return m_data; }
    inline const StackValue *GetData() const { return m_data; }
    inline size_t GetStackPointer() const { return m_sp; }

    inline StackValue &operator[](size_t index)
    {
        ASSERT_MSG(index < stack_size, "out of bounds");
        return m_data[index];
    }

    inline const StackValue &operator[](size_t index) const
    {
        ASSERT_MSG(index < stack_size, "out of bounds");
        return m_data[index];
    }

    // return the top value from the stack
    inline StackValue &Top()
    {
        ASSERT_MSG(m_sp > 0, "stack underflow");
        return m_data[m_sp - 1];
    }

    // return the top value from the stack
    inline const StackValue &Top() const
    {
        ASSERT_MSG(m_sp > 0, "stack underflow");
        return m_data[m_sp - 1];
    }

    // push a value to the stack
    inline void Push(const StackValue &value)
    {
        ASSERT_MSG(m_sp < stack_size, "stack overflow");
        m_data[m_sp++] = value;
    }

    // pop top value from the stack
    inline void Pop()
    {
        ASSERT_MSG(m_sp > 0, "stack underflow");
        m_sp--;
    }

private:
    StackValue *m_data;
    size_t m_sp;
};

#endif
