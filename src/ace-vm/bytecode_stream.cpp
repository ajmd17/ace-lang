#include <ace-vm/bytecode_stream.hpp>

namespace ace {
namespace vm {

BytecodeStream::BytecodeStream()
    : m_buffer(non_owning_ptr<char>()),
      m_size(0),
      m_position(0)
{
}

BytecodeStream::BytecodeStream(const non_owning_ptr<char> &buffer, size_t size, size_t position)
    : m_buffer(buffer),
      m_size(size),
      m_position(position)
{
}


BytecodeStream::BytecodeStream(const BytecodeStream &other)
    : m_buffer(other.m_buffer),
      m_size(other.m_size),
      m_position(other.m_position)
{
}

BytecodeStream &BytecodeStream::operator=(const BytecodeStream &other)
{
    m_buffer = other.m_buffer;
    m_size = other.m_size;
    m_position = other.m_position;

    return *this;
}

} // namespace vm
} // namespace ace