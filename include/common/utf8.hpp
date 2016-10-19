#ifndef UTF8_HPP
#define UTF8_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <stdint.h>
#include <string.h>

#ifdef __MINGW32__
#undef _WIN32
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif

#ifdef _WIN32
typedef std::wostream utf8_ostream;
static utf8_ostream &ucout = std::wcout;
#else
typedef std::ostream utf8_ostream;
static utf8_ostream &ucout = std::cout;
#endif

typedef uint32_t u32char;

inline void utf8_init()
{
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_U16TEXT);
#endif
}

inline char *utf32_get_bytes(u32char &ch)
{
    return reinterpret_cast<char*>(&ch);
}

inline bool utf32_isspace(u32char ch)
{
    return ch == (u32char)' '  ||
           ch == (u32char)'\n' ||
           ch == (u32char)'\t' ||
           ch == (u32char)'\r';
}

inline bool utf32_isdigit(u32char ch)
{
    return ((u32char)ch >= '0') && ((u32char)ch <= '9');
}

inline bool utf32_isalpha(u32char ch)
{
    return (ch >= 0xC0) || ((ch >= (u32char)'A' && ch <= (u32char)'Z') ||
        (ch >= (u32char)'a' && ch <= (u32char)'z'));
}

int utf8_strlen(const char *str);
int utf32_strlen(const u32char *str);
int utf8_strcmp(const char *s1, const char *s2);
int utf32_strcmp(const u32char *lhs, const u32char *rhs);
char *utf8_strcpy(char *dst, const char *src);
u32char *utf32_strcpy(u32char *dst, const u32char *src);
char *utf8_strncpy(char *dst, const char *src, size_t n);
u32char *utf32_strncpy(u32char *s1, const u32char *s2, size_t n);
char *utf8_strcat(char *dst, const char *src);
u32char *utf32_strcat(u32char *dst, const u32char *src);
u32char char8to32(const char *str);
void char32to8(u32char src, char *dst);
void utf8to32(const char *src, u32char *dst, int size);
u32char utf8_charat(const char *str, int index);
void utf8_charat(const char *str, char *dst, int index);

class Utf8String {
public:
    Utf8String();
    explicit Utf8String(size_t size);
    Utf8String(const char *str);
    Utf8String(const Utf8String &other);
    ~Utf8String();

    inline char *GetData() const { return m_data; }
    inline size_t GetBufferSize() const { return m_size; }
    inline size_t GetLength() const { return m_length; }

    Utf8String &operator=(const char *str);
    Utf8String &operator=(const Utf8String &other);

	inline bool operator==(const char *str) const { return !(strcmp(m_data, str)); }
	inline bool operator==(const Utf8String &other) const { return !(strcmp(m_data, other.m_data)); }
	inline bool operator<(const char *str) const { return (utf8_strcmp(m_data, str) == -1); }
	inline bool operator<(const Utf8String &other) const { return (utf8_strcmp(m_data, other.m_data) == -1); }
	inline bool operator<=(const char *str) const { return !(strcmp(m_data, str)) || (utf8_strcmp(m_data, str) == -1); }
	inline bool operator<=(const Utf8String &other) const { return !(strcmp(m_data, other.m_data)) || (utf8_strcmp(m_data, other.m_data) == -1); }
	inline bool operator>(const char *str) const { return (utf8_strcmp(m_data, str) == 1); }
	inline bool operator>(const Utf8String &other) const { return (utf8_strcmp(m_data, other.m_data) == 1); }
	inline bool operator>=(const char *str) const { return !(strcmp(m_data, str)) || (utf8_strcmp(m_data, str) == 1); }
	inline bool operator>=(const Utf8String &other) const { return !(strcmp(m_data, other.m_data)) || (utf8_strcmp(m_data, other.m_data) == 1); }

    Utf8String operator+(const char *str) const;
    Utf8String operator+(const Utf8String &other) const;
    Utf8String &operator+=(const char *str);
    Utf8String &operator+=(const Utf8String &other);
    u32char operator[](size_t index) const;

    friend utf8_ostream &operator<<(utf8_ostream &os, const Utf8String &str);

private:
    char *m_data;
    size_t m_size; // buffer size (not length)
    size_t m_length;
};

#endif
