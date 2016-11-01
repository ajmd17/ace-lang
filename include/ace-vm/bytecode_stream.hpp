#ifndef BYTECODE_STREAM_HPP
#define BYTECODE_STREAM_HPP

#include <iostream>
#include <cassert>

class BytecodeStream {
public:
    BytecodeStream(char *buffer, size_t size);
    BytecodeStream(const BytecodeStream &other) = delete;

    inline void ReadBytes(char *ptr, size_t num_bytes)
    {
        assert(m_position + num_bytes < m_size + 1 && "cannot read past end of buffer");
        for (size_t i = 0; i < num_bytes; i++) {
            ptr[i] = m_buffer[m_position++];
        }
    }

    template <typename T>
    inline void Read(T *ptr, size_t num_bytes = sizeof(T))
    {
        ReadBytes(reinterpret_cast<char*>(ptr), num_bytes);
    }

    inline size_t Position() const { return m_position; }
    inline size_t Size() const { return m_size; }
    inline void Seek(size_t address) { m_position = address; }
    inline void Skip(size_t amount) { m_position += amount; }
    inline bool Eof() const { return m_position >= m_size; }

private:
    char *m_buffer;
    size_t m_size;
    size_t m_position;
};

#endif
