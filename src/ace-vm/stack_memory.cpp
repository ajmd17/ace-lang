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

void Stack::MarkAll()
{
    for (int i = m_sp - 1; i >= 0; i--) {
        m_data[i].Mark();
    }
}