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

u32char SourceStream::Peek() const 
{
    size_t pos = m_position;
    if (pos >= m_file->GetSize()) {
        return '\0';
    }

    // the current character
    char ch = m_file->GetBuffer()[pos];

    // the character as a utf-32 character
    u32char u32_ch = 0;
    char *u32_bytes = reinterpret_cast<char*>(&u32_ch);

    // check to see if it is a utf-8 character
    unsigned char uc = (unsigned char)ch;
    if (uc >= 0 && uc <= 127) {
        // 1-byte character
        u32_bytes[0] = ch;
    } else if ((uc & 0xE0) == 0xC0) {
        // 2-byte character
        u32_bytes[0] = ch;
        u32_bytes[1] = m_file->GetBuffer()[pos + 1];
    } else if ((uc & 0xF0) == 0xE0) {
        // 3-byte character
        u32_bytes[0] = ch;
        u32_bytes[1] = m_file->GetBuffer()[pos + 1];
        u32_bytes[2] = m_file->GetBuffer()[pos + 2];
    } else if ((uc & 0xF8) == 0xF0) {
        // 4-byte character
        u32_bytes[0] = ch;
        u32_bytes[1] = m_file->GetBuffer()[pos + 1];
        u32_bytes[2] = m_file->GetBuffer()[pos + 2];
        u32_bytes[3] = m_file->GetBuffer()[pos + 3];
    } else {
        // invalid utf-8
        u32_ch = (u32char)'\0';
    }

    return u32_ch;
}

u32char SourceStream::Next() 
{
    int tmp;
    return Next(tmp);
}

u32char SourceStream::Next(int &pos_change)
{
    int pos_before = m_position;

    if (m_position >= m_file->GetSize()) {
        return '\0';
    }

    // the current character
    char ch = m_file->GetBuffer()[m_position++];

    // the character as a utf-32 character
    u32char u32_ch = 0;
    char *u32_bytes = reinterpret_cast<char*>(&u32_ch);

    // check to see if it is a utf-8 character
    unsigned char uc = (unsigned char)ch;
    if (uc >= 0 && uc <= 127) {
        // 1-byte character
        u32_bytes[0] = ch;
    } else if ((uc & 0xE0) == 0xC0) {
        // 2-byte character
        u32_bytes[0] = ch;
        u32_bytes[1] = m_file->GetBuffer()[m_position++];
    } else if ((uc & 0xF0) == 0xE0) {
        // 3-byte character
        u32_bytes[0] = ch;
        u32_bytes[1] = m_file->GetBuffer()[m_position++];
        u32_bytes[2] = m_file->GetBuffer()[m_position++];
    } else if ((uc & 0xF8) == 0xF0) {
        // 4-byte character
        u32_bytes[0] = ch;
        u32_bytes[1] = m_file->GetBuffer()[m_position++];
        u32_bytes[2] = m_file->GetBuffer()[m_position++];
        u32_bytes[3] = m_file->GetBuffer()[m_position++];
    } else {
        // invalid utf-8
        u32_ch = (u32char)'\0';
    }

    pos_change = m_position - pos_before;

    return u32_ch;
}

void SourceStream::GoBack(int n)
{
    if (((int)m_position - n) < 0) {
        throw std::out_of_range("not large enough to go back");
    }
    m_position -= n;
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
