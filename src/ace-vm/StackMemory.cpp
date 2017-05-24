#include <ace-vm/StackMemory.hpp>

#include <iomanip>

namespace ace {
namespace vm {

const uint16_t Stack::STACK_SIZE = 20000;

std::ostream &operator<<(std::ostream &os, const Stack &stack)
{
    // print table header
    os << std::left;
    os << std::setw(5) << "Index" << "| ";
    os << std::setw(18) << "Type" << "| ";
    os << std::setw(16) << "Value";
    os << std::endl;

    for (int i = 0; i < stack.m_sp; i++) {
        Value &value = stack.m_data[i];
        
        os << std::setw(5) << i << "| ";


        if (value.GetType() == Value::ValueType::HEAP_POINTER) {
            os << "-> " << std::setw(15) << value.GetTypeString() << "| ";

            os << std::setw(16);
            os << (void*)value.m_value.ptr;
        } else {
            os << std::setw(18);
            os << value.GetTypeString() << "| ";

            os << std::setw(16);
            utf::Utf8String tmp_str(256);
            value.ToRepresentation(tmp_str, false);
            os << tmp_str;
        }

        os << std::endl;
    }
    return os;
}

Stack::Stack()
    : m_data(new Value[STACK_SIZE]),
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

} // namespace vm
} // namespace ace