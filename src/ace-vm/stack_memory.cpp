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
