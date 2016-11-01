#include <ace-vm/static_memory.hpp>

const uint16_t StaticMemory::static_size = 1000;

StaticMemory::StaticMemory()
    : m_data(new StackValue[static_size]),
      m_sp(0)
{
}

StaticMemory::~StaticMemory()
{
    // delete all heap allocated objects
    for (; m_sp != 0; m_sp--) {
        StackValue &sv = m_data[m_sp - 1];
        if (sv.m_type == StackValue::HEAP_POINTER &&
            sv.m_value.ptr != nullptr) {
            delete sv.m_value.ptr;
        }
    }

    delete[] m_data;
}
