#include <ace-vm/stack_memory.hpp>

const uint16_t Stack::stack_size = 20000;

Stack::Stack()
    : m_data(new StackValue[stack_size]),
      m_sp(0)
{
}

Stack::~Stack()
{
    delete[] m_data;
}

void Stack::Purge()
{
    // just set stack pointer to zero
    // heap allocated objects are not owned,
    // so we don't have to delete them
    // they will be deleted either by the destructor of the `Heap` class,
    // or by the `Purge` function of the `Heap` class.
    m_sp = 0;
}

void Stack::MarkAll()
{
    for (int i = m_sp - 1; i >= 0; i--) {
        m_data[i].Mark();
    }
}