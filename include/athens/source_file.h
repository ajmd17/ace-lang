#ifndef SOURCE_FILE_H
#define SOURCE_FILE_H

#include <cstddef>

class SourceFile {
public:
    SourceFile(size_t size);
    SourceFile(const SourceFile &other) = delete;
    ~SourceFile();

    SourceFile &operator=(const SourceFile &other) = delete;

    inline char *GetBuffer() const { return m_buffer; }
    inline size_t GetSize() const { return m_size; }

private:
    char *m_buffer;
    size_t m_size;
};

#endif
