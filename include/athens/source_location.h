#ifndef SOURCE_LOCATION_H
#define SOURCE_LOCATION_H

#include <string>

class SourceLocation {
public:
    SourceLocation(int line, int column, const std::string &filename = "");
    SourceLocation(const SourceLocation &other);

    inline int GetLine() const { return m_line; }
    inline int GetColumn() const { return m_column; }
    inline const std::string &GetFileName() const { return m_filename; }

    bool operator<(const SourceLocation &other) const;

private:
    int m_line;
    int m_column;
    std::string m_filename;
};

#endif