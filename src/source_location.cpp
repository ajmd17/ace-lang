#include <athens/source_location.h>

SourceLocation::SourceLocation(int line, int column, const std::string &filename)
    : m_line(line),
      m_column(column),
      m_filename(filename)
{
}

SourceLocation::SourceLocation(const SourceLocation &other)
    : m_line(other.m_line),
      m_column(other.m_column),
      m_filename(other.m_filename)
{
}

bool SourceLocation::operator<(const SourceLocation &other) const
{
    if (m_filename == other.m_filename) {
        if (m_line == other.m_line) {
            return m_column < other.m_column;
        } else {
            return m_line < other.m_line;
        }
    } else {
        return m_filename < other.m_filename;
    }
}