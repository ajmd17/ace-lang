#include <ace-vm/stack_value.hpp>
#include <ace-vm/object.hpp>
#include <ace-vm/array.hpp>

StackValue::StackValue()
    : m_type(HEAP_POINTER)
{
    // initialize to null reference
    m_value.ptr = nullptr;
}

StackValue::StackValue(const StackValue &other)
    : m_type(other.m_type),
      m_value(other.m_value)
{
}

void StackValue::Mark()
{
    if (m_type == StackValue::HEAP_POINTER && m_value.ptr != nullptr) {
        Object *objptr = nullptr;
        Array *arrayptr = nullptr;

        if ((objptr = m_value.ptr->GetPointer<Object>()) != nullptr) {
            int objsize = objptr->GetSize();
            for (int i = 0; i < objsize; i++) {
                objptr->GetMember(i).Mark();
            }
        } else if ((arrayptr = m_value.ptr->GetPointer<Array>()) != nullptr) {
            int arraysize = arrayptr->GetSize();
            for (int i = 0; i < arraysize; i++) {
                arrayptr->AtIndex(i).Mark();
            }
        }

        m_value.ptr->GetFlags() |= GC_MARKED;
    }
}