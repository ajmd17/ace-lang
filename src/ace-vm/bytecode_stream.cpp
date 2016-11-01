#include <ace-vm/bytecode_stream.hpp>

BytecodeStream::BytecodeStream(char *buffer, size_t size)
    : m_buffer(buffer),
      m_size(size),
      m_position(0)
{
}
