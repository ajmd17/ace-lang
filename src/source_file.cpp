#include <athens/source_file.h>

#include <stdexcept>
#include <cstring>

SourceFile::SourceFile(size_t size)
    : m_position(0),
      m_size(size)
{
    m_buffer = new char[m_size];
    std::memset(m_buffer, '\0', m_size);
}

SourceFile::~SourceFile()
{
    delete[] m_buffer;
}

SourceFile &SourceFile::operator>>(const std::string &str)
{
    size_t length = str.length();
    // make sure we have enough space in the buffer
    if (m_position + length >= m_size) {
        throw std::out_of_range("not enough space in buffer");
    }

    for (size_t i = 0; i < length; i++) {
        m_buffer[m_position++] = str[i];
    }

    return *this;
}