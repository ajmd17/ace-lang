#ifndef SOURCE_STREAM_H
#define SOURCE_STREAM_H

#include <athens/source_file.h>
#include <common/utf8.h>

#include <cstddef>

class SourceStream {
public:
    SourceStream(SourceFile *file);
    SourceStream(const SourceStream &other);

    inline SourceFile *GetFile() const { return m_file; }
    inline size_t GetPosition() const { return m_position; }
    inline bool HasNext() const { return m_position < m_file->GetSize(); }
    u32char Peek() const;
    u32char Next();
    u32char Next(int &pos_change);
    void GoBack(int n = 1);
    void Read(char *ptr, size_t num_bytes);

private:
    SourceFile *m_file;
    size_t m_position;
};

#endif
