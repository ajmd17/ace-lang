#include <ace-c/dis/byte_stream.hpp>

#include <stdexcept>

ByteStream::ByteStream(SourceFile *file)
    : m_file(file),
      m_position(0)
{
}

ByteStream::ByteStream(const ByteStream &other)
    : m_file(other.m_file),
      m_position(other.m_position)
{
}

char ByteStream::Peek() const
{
    if (m_position >= m_file->GetSize()) {
        return '\0';
    }

    // the current character
    return m_file->GetBuffer()[m_position];
}

char ByteStream::Next()
{
    if (m_position >= m_file->GetSize()) {
        return '\0';
    }

    // the current character
    return m_file->GetBuffer()[m_position++];
}

void ByteStream::ReadBytes(char *ptr, size_t num_bytes)
{
    for (size_t i = 0; i < num_bytes; i++) {
        if (m_position >= m_file->GetSize()) {
            throw std::out_of_range("attempted to read past the limit");
        }
        ptr[i] = m_file->GetBuffer()[m_position++];
    }
}
