#include <ace-vm/bytecode_stream.hpp>

BytecodeStream::BytecodeStream(char *buffer, size_t size, size_t position)
    : m_buffer(buffer),
      m_size(size),
      m_position(position)
{
}
