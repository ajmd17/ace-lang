#ifndef BYTE_STREAM_HPP
#define BYTE_STREAM_HPP

#include <ace-c/SourceFile.hpp>

#include <cstddef>

class ByteStream {
public:
    ByteStream(SourceFile *file);
    ByteStream(const ByteStream &other);

    inline SourceFile *GetFile() const { return m_file; }
    inline size_t GetPosition() const { return m_position; }
    inline bool HasNext() const { return m_position < m_file->GetSize(); }
    char Peek() const;
    char Next();
    void ReadBytes(char *ptr, size_t num_bytes);

    template <typename T>
    inline void Read(T *ptr, size_t num_bytes = sizeof(T))
    {
        ReadBytes(reinterpret_cast<char*>(ptr), num_bytes);
    }

private:
    SourceFile *m_file;
    size_t m_position;
};

#endif
