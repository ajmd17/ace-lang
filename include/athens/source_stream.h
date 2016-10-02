#ifndef SOURCE_STREAM_H
#define SOURCE_STREAM_H

#include <athens/source_file.h>

#include <cstddef>

class SourceStream {
public:
    SourceStream(SourceFile *file);
    SourceStream(const SourceStream &other);

    inline SourceFile *GetFile() const { return m_file; }
    inline size_t GetPosition() const { return m_position; }
    inline bool HasNext() const { return m_position < m_file->GetSize(); }
    inline char Peek(int n = 0) const 
    {
        size_t pos = m_position + n;
        if (pos >= m_file->GetSize()) {
            return '\0';
        }
        return m_file->GetBuffer()[pos]; 
    }
    
    char Next();
    void Read(char *ptr, size_t num_bytes);

private:
    SourceFile *m_file;
    size_t m_position;
};

#endif
