#include <ace-vm/Array.hpp>

#include <cmath>
#include <cstring>

namespace ace {
namespace vm {

Array::Array(int size)
    : m_size(size),
      m_capacity(1 << (unsigned int)std::ceil(std::log(size) / std::log(2.0))),
      m_buffer(new Value[m_capacity])
{
}

Array::Array(const Array &other)
    : m_size(other.m_size),
      m_capacity(other.m_capacity),
      m_buffer(new Value[other.m_capacity])
{
    // copy all members
    for (int i = 0; i < m_size; i++) {
        m_buffer[i] = other.m_buffer[i];
    }
}

Array::~Array()
{
    delete[] m_buffer;
}

Array &Array::operator=(const Array &other)
{
    if (m_buffer) {
        delete[] m_buffer;
    }

    m_size = other.m_size;
    m_capacity = other.m_capacity;
    m_buffer = new Value[other.m_capacity];

    // copy all objects
    for (int i = 0; i < m_size; i++) {
        m_buffer[i] = other.m_buffer[i];
    }

    return *this;
}

void Array::Push(const Value &value)
{
    int index = m_size;
    if (index >= m_capacity) {
        // delete and copy all over again
        m_capacity = 1 << (unsigned int)std::ceil(std::log(m_size + 1) / std::log(2.0));
        Value *new_buffer = new Value[m_capacity];
        // copy all objects into new buffer
        for (int i = 0; i < m_size; i++) {
            new_buffer[i] = m_buffer[i];
        }
        // delete old buffer
        if (m_buffer) {
            delete[] m_buffer;
        }
        // set internal buffer to the new one
        m_buffer = new_buffer;
    }
    // set item at index
    m_buffer[index] = value;
    m_size++;
}

void Array::Pop()
{
    m_size--;
}

} // namespace vm
} // namespace ace