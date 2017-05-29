#include <ace-vm/Array.hpp>

#include <common/my_assert.hpp>

#include <cmath>
#include <cstring>

namespace ace {
namespace vm {

Array::Array(size_t size)
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
    for (size_t i = 0; i < m_size; i++) {
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
    for (size_t i = 0; i < m_size; i++) {
        m_buffer[i] = other.m_buffer[i];
    }

    return *this;
}

void Array::Push(const Value &value)
{
    size_t index = m_size;
    if (index >= m_capacity) {
        // delete and copy all over again
        m_capacity = 1 << (unsigned int)std::ceil(std::log(m_size + 1) / std::log(2.0));
        Value *new_buffer = new Value[m_capacity];
        // copy all objects into new buffer
        for (size_t i = 0; i < m_size; i++) {
            new_buffer[i] = m_buffer[i];
        }
        // delete old buffer
        if (m_buffer != nullptr) {
            delete[] m_buffer;
        }
        // set internal buffer to the new one
        m_buffer = new_buffer;
    }
    // set item at index
    m_buffer[index] = value;
    m_size++;
}

void Array::PushMany(size_t n, Value **values)
{
    size_t index = m_size;
    if (index + n - 1 >= m_capacity) {
        // delete and copy all over again
        m_capacity = 1 << (unsigned int)std::ceil(std::log(m_size + n) / std::log(2.0));
        Value *new_buffer = new Value[m_capacity];
        // copy all objects into new buffer
        for (size_t i = 0; i < m_size; i++) {
            new_buffer[i] = m_buffer[i];
        }
        // delete old buffer
        if (m_buffer != nullptr) {
            delete[] m_buffer;
        }
        // set internal buffer to the new one
        m_buffer = new_buffer;
    }

    for (size_t i = 0; i < n; i++) {
        ASSERT(values[i] != nullptr);
        // set item at index
        m_buffer[index + i] = *values[i];
    }

    m_size += n;
}

void Array::Pop()
{
    m_size--;
}

void Array::GetRepresentation(utf::Utf8String &out_str, bool add_type_name) const
{
    // convert array list to string
    const char sep_str[] = ", ";

    utf::Utf8String res("[", 256);

    // convert all array elements to string
    for (size_t i = 0; i < m_size; i++) {
        m_buffer[i].ToRepresentation(res, add_type_name);

        if (i != m_size - 1) {
            res += sep_str;
        }
    }

    res += "]";

    out_str += res;
}

} // namespace vm
} // namespace ace