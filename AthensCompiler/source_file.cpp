#include "source_file.h"

SourceFile::SourceFile(size_t size)
    : m_size(size)
{
    m_buffer = new char[m_size];
}

SourceFile::~SourceFile()
{
    delete[] m_buffer;
}