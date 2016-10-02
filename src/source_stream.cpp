#include <athens/source_stream.h>

#include <stdexcept>

SourceStream::SourceStream(SourceFile *file)
    : m_file(file),
      m_position(0)
{
}

SourceStream::SourceStream(const SourceStream &other)
    : m_file(other.m_file),
      m_position(other.m_position)
{
}

char SourceStream::Next() 
{
    if (m_position >= m_file->GetSize()) {
        throw std::out_of_range("attempted to read past the limit");
    }
    return m_file->GetBuffer()[m_position++];
}

void SourceStream::Read(char *ptr, size_t num_bytes)
{
    for (size_t i = 0; i < num_bytes; i++) {
        if (m_position >= m_file->GetSize()) {
            throw std::out_of_range("attempted to read past the limit");
        }
        ptr[i] = m_file->GetBuffer()[m_position++];
    }
}
