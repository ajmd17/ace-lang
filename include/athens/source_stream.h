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

    void Read(char *ptr, size_t num_bytes);

private:
    SourceFile *m_file;
    size_t m_position;
};

#endif
