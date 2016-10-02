#ifndef SOURCE_LOCATION_H
#define SOURCE_LOCATION_H

#include <string>

class SourceLocation {
public:
    static const SourceLocation eof;

public:
    SourceLocation(int line = 0, int column = 0, const std::string &filename = "");
    SourceLocation(const SourceLocation &other);

    inline int GetLine() const { return m_line; }
    inline int &GetLine() { return m_line; }
    inline int GetColumn() const { return m_column; }
    inline int &GetColumn() { return m_column; }
    inline const std::string &GetFileName() const { return m_filename; }

    bool operator<(const SourceLocation &other) const;

private:
    int m_line;
    int m_column;
    std::string m_filename;
};

#endif