#ifndef STACK_MEMORY_HPP
#define STACK_MEMORY_HPP

#include <ace-vm/stack_value.hpp>

#include <array>
#include <cassert>

class Stack {
public:
    static const uint16_t stack_size;

public:
    Stack();
    Stack(const Stack &other) = delete;
    ~Stack();

    inline StackValue *GetData() { return m_data; }
    inline const StackValue *GetData() const { return m_data; }
    inline size_t GetStackPointer() const { return m_sp; }
    
    void MarkAll();

    inline StackValue &operator[](size_t index)
    {
        assert(index < stack_size && "out of bounds");
        return m_data[index];
    }

    inline const StackValue &operator[](size_t index) const
    {
        assert(index < stack_size && "out of bounds");
        return m_data[index];
    }

    // return the top value from the stack
    inline StackValue &Top()
    {
        assert(m_sp > 0 && "stack underflow");
        return m_data[m_sp - 1];
    }

    // return the top value from the stack
    inline const StackValue &Top() const
    {
        assert(m_sp > 0 && "stack underflow");
        return m_data[m_sp - 1];
    }

    // push a value to the stack
    inline void Push(const StackValue &value)
    {
        assert(m_sp < stack_size && "stack overflow");
        m_data[m_sp++] = value;
    }

    // pop top value from the stack
    inline void Pop()
    {
        assert(m_sp > 0 && "stack underflow");
        m_sp--;
    }

private:
    StackValue *m_data;
    size_t m_sp;
};

#endif
