#include <common/utf8.h> 

int utf8_strlen(const char *str)
{
    int max = strlen(str);
    int count = 0;

    for (int i = 0; i < max; i++, count++) {
        unsigned char c = (unsigned char)str[i];
        if (c >= 0 && c <= 127) i += 0;
        else if ((c & 0xE0) == 0xC0) i += 1;
        else if ((c & 0xF0) == 0xE0) i += 2;
        else if ((c & 0xF8) == 0xF0) i += 3;
        else return -1;//invalid utf8
    }

    return count;
}

int utf32_strlen(const u32char *str)
{
    int counter = 0;
    const u32char *pos = str;
    for (; *pos; ++pos, counter++);
    return counter;
}

int utf8_strcmp(const char *s1, const char *s2)
{
    for (; *s1 || *s2;) {
        unsigned char c;

        u32char c1 = 0;
        char *c1_bytes = reinterpret_cast<char*>(&c1);

        u32char c2 = 0;
        char *c2_bytes = reinterpret_cast<char*>(&c2);

        // get the character for s1
        c = (unsigned char)*s1;

        if (c >= 0 && c <= 127) {
            c1_bytes[0] = *(s1++);
        } else if ((c & 0xE0) == 0xC0) {
            c1_bytes[0] = *(s1++);
            c1_bytes[1] = *(s1++);
        } else if ((c & 0xF0) == 0xE0) {
            c1_bytes[0] = *(s1++);
            c1_bytes[1] = *(s1++);
            c1_bytes[2] = *(s1++);
        } else if ((c & 0xF8) == 0xF0) {
            c1_bytes[0] = *(s1++);
            c1_bytes[1] = *(s1++);
            c1_bytes[2] = *(s1++);
            c1_bytes[3] = *(s1++);
        }

        // get the character for s2
        c = (unsigned char)*s2;

        if (c >= 0 && c <= 127) {
            c2_bytes[0] = *(s2++);
        } else if ((c & 0xE0) == 0xC0) {
            c2_bytes[0] = *(s2++);
            c2_bytes[1] = *(s2++);
        } else if ((c & 0xF0) == 0xE0) {
            c2_bytes[0] = *(s2++);
            c2_bytes[1] = *(s2++);
            c2_bytes[2] = *(s2++);
        } else if ((c & 0xF8) == 0xF0) {
            c2_bytes[0] = *(s2++);
            c2_bytes[1] = *(s2++);
            c2_bytes[2] = *(s2++);
            c2_bytes[3] = *(s2++);
        }

        if (c1 < c2) {
            return -1;
        } else if (c1 > c2) {
            return 1;
        }
    }

    return 0;
}

int utf32_strcmp(const u32char *lhs, const u32char *rhs)
{
    const u32char *s1 = lhs;
    const u32char *s2 = rhs;

    for (; *s1 || *s2; s1++, s2++) {
        if (*s1 < *s2) {
            return -1;
        } else if (*s1 > *s2) {
            return 1;
        }
    }

    return 0;
}

char *utf8_strcpy(char *dst, const char *src)
{
    // copy all raw bytes   
    for (int i = 0; (dst[i] = src[i]) != '\0'; i++);
    return dst;
}

u32char *utf32_strcpy(u32char *dst, const u32char *src)
{
    for (int i = 0; (dst[i] = src[i]) != '\0'; i++);
    return dst;
}

char *utf8_strncpy(char *dst, const char *src, size_t n)
{
    size_t i = 0;
    size_t count = 0;
    
    size_t max = strlen(dst) + 1;

    for (; src[i] != '\0'; i++, count++) {

        if (count == n) {
            // finished copying, jump to end
            break;
        }

        unsigned char c = (unsigned char)src[i];

        if (c >= 0 && c <= 127) {
            dst[i] = src[i];
        } else if ((c & 0xE0) == 0xC0) {
            dst[i] = src[i];
            dst[i + 1] = src[i + 1];
            i += 1;
        } else if ((c & 0xF0) == 0xE0) {
            dst[i] = src[i];
            dst[i + 1] = src[i + 1];
            dst[i + 2] = src[i + 2];
            i += 2;
        } else if ((c & 0xF8) == 0xF0) {
            dst[i] = src[i];
            dst[i + 1] = src[i + 1];
            dst[i + 2] = src[i + 2];
            dst[i + 3] = src[i + 3];
            i += 3;
        } else { 
            break; // invalid utf-8
        }
    }

    // fill rest with NUL bytes
    for (; i < max; i++) {
        dst[i] = '\0';
    }

    return dst;
}

u32char *utf32_strncpy(u32char *s1, const u32char *s2, size_t n)
{
    u32char *s = s1;
    while (n > 0 && *s2 != '\0') {
        *s++ = *s2++;
        --n;
    }
    while (n > 0) {
        *s++ = '\0';
        --n;
    }
    return s1;
}

char *utf8_strcat(char *dst, const char *src)
{
    while (*dst) {
        dst++;
    }
    while (*dst++ = *src++);
    return --dst;
}

u32char *utf32_strcat(u32char *dst, const u32char *src)
{
    while (*dst) {
        dst++;
    }
    while (*dst++ = *src++);
    return --dst;
}

u32char char8to32(const char *str)
{
    int num_bytes = strlen(str);
    if (num_bytes > sizeof(u32char)) {
        // not wide enough to store the character
        return -1;
    }
    
    u32char result = 0;

    char *result_bytes = reinterpret_cast<char*>(&result);
    for (int i = 0; i < num_bytes; i++) {
        result_bytes[i] = str[i];
    }

    return result;
}

void char32to8(u32char src, char *dst)
{
    char *src_bytes = reinterpret_cast<char*>(&src);

    for (int i = 0; i < sizeof(u32char); i++) {
        if (src_bytes[i] == 0) {
            // stop reading
            break;
        }

        dst[i] = src_bytes[i];
    }
}

void utf8to32(const char *src, u32char *dst, int size)
{
    int max = size * (int)sizeof(u32char);
    char *dst_bytes = reinterpret_cast<char*>(dst);

    const char *pos = src;

    for (int i = 0; *pos && i < max; i += sizeof(u32char)) {
        unsigned char c = (unsigned char)*pos;

        if (c >= 0 && c <= 127) {
            dst_bytes[i] = *(pos++);
        } else if ((c & 0xE0) == 0xC0) {
            dst_bytes[i] = *(pos++);
            dst_bytes[i + 1] = *(pos++);
        } else if ((c & 0xF0) == 0xE0) {
            dst_bytes[i] = *(pos++);
            dst_bytes[i + 1] = *(pos++);
            dst_bytes[i + 2] = *(pos++);
        } else if ((c & 0xF8) == 0xF0) {
            dst_bytes[i] = *(pos++);
            dst_bytes[i + 1] = *(pos++);
            dst_bytes[i + 2] = *(pos++);
            dst_bytes[i + 3] = *(pos++);
        } else {
            // invalid utf-8
            break;
        }
    }
}

u32char utf8_charat(const char *str, int index)
{
    int max = strlen(str);
    int count = 0;

    for (int i = 0; i < max; i++, count++) {
        unsigned char c = (unsigned char)str[i];

        u32char ret = 0;
        char *ret_bytes = reinterpret_cast<char*>(&ret);

        if (c >= 0 && c <= 127) {
            ret_bytes[0] = str[i];
        } else if ((c & 0xE0) == 0xC0) {
            ret_bytes[0] = str[i];
            ret_bytes[1] = str[i + 1];
            i += 1;
        } else if ((c & 0xF0) == 0xE0) {
            ret_bytes[0] = str[i];
            ret_bytes[1] = str[i + 1];
            ret_bytes[2] = str[i + 2];
            i += 2;
        } else if ((c & 0xF8) == 0xF0) {
            ret_bytes[0] = str[i];
            ret_bytes[1] = str[i + 1];
            ret_bytes[2] = str[i + 2];
            ret_bytes[3] = str[i + 3];
            i += 3;
        } else {
            // invalid utf-8
            break;
        }

        if (index == count) {
            // reached index
            return ret;
        }
    }

    // error
    return -1;
}

void utf8_charat(const char *str, char *dst, int index)
{
    char32to8(utf8_charat(str, index), dst);
}

Utf8String::Utf8String()
    : m_data(new char[1]),
      m_size(1),
      m_length(0)
{
    m_data[0] = '\0';
}

Utf8String::Utf8String(size_t size)
    : m_data(new char[size + 1]),
      m_size(size + 1),
      m_length(0)
{
    memset(m_data, 0, m_size);
}

Utf8String::Utf8String(const char *str)
{
    if (str == nullptr) {
        m_data = new char[1];
        m_data[0] = '\0';
        m_size = 1;
        m_length = 0;
    } else {
        // copy raw bytes
        m_size = strlen(str) + 1;
        m_data = new char[m_size];
        strcpy(m_data, str);
        // recalculate length
        m_length = utf8_strlen(m_data);
    }
}

Utf8String::Utf8String(const Utf8String &other)
{
    // copy raw bytes
    m_size = strlen(other.m_data) + 1;
    m_data = new char[m_size];
    strcpy(m_data, other.m_data);
    m_length = other.m_length;
}

Utf8String::~Utf8String()
{
    if (m_data != nullptr) {
        delete[] m_data;
    }
}

Utf8String &Utf8String::operator=(const char *str)
{
    if (m_data != nullptr) {
        delete[] m_data;
    }

    if (str == nullptr) {
        m_data = new char[1];
        m_data[0] = '\0';
        m_size = 1;
        m_length = 0;
    } else {
        // copy raw bytes
        m_size = strlen(str) + 1;
        m_data = new char[m_size];
        strcpy(m_data, str);
        // recalculate length
        m_length = utf8_strlen(m_data);
    }

    return *this;
}

Utf8String &Utf8String::operator=(const Utf8String &other)
{
    if (m_data != nullptr) {
        delete[] m_data;
    }
    
    // copy raw bytes
    m_size = strlen(other.m_data) + 1;
    m_data = new char[m_size];
    strcpy(m_data, other.m_data);
    m_length = other.m_length;

    return *this;
}

Utf8String Utf8String::operator+(const char *str) const
{
    Utf8String result(m_length + strlen(str));

    utf8_strcpy(result.m_data, m_data);
    utf8_strcat(result.m_data, str);

    // calculate length
    result.m_length = utf8_strlen(result.m_data);

    return result;
}

Utf8String Utf8String::operator+(const Utf8String &other) const
{
    Utf8String result(m_length + other.m_length);

    utf8_strcpy(result.m_data, m_data);
    utf8_strcat(result.m_data, other.m_data);

    // calculate length
    result.m_length = utf8_strlen(result.m_data);

    return result;
}

Utf8String &Utf8String::operator+=(const char *str)
{
    size_t this_len = strlen(m_data);
    size_t other_len = strlen(str);

    if (this_len + other_len < m_size) {
        strcat(m_data, str);
        m_size += other_len;
        // calculate utf-8 length of string and add it
        m_length += utf8_strlen(str);
    } else {
        // we must delete and recreate the array
        m_size = this_len + other_len + 1;

        char *new_data = new char[m_size];
        strcpy(new_data, m_data);
        strcat(new_data, str);
        delete[] m_data;
        m_data = new_data;
        // recalculate length
        m_length = utf8_strlen(m_data);
    }

    return *this;
}

Utf8String &Utf8String::operator+=(const Utf8String &other)
{
    return operator+=(other.m_data);
}

u32char Utf8String::operator[](size_t index) const
{
    u32char result;
    if (m_data == nullptr || ((result = utf8_charat(m_data, index) == (u32char(-1))))) {
        throw std::out_of_range("index out of range");
    }
    return result;
}

utf8_ostream &operator<<(utf8_ostream &os, const Utf8String &str)
{
#ifdef _WIN32
    std::vector<wchar_t> buffer;
    buffer.resize(MultiByteToWideChar(CP_UTF8, 0, str.m_data, -1, 0, 0));
    MultiByteToWideChar(CP_UTF8, 0, str.m_data, -1, &buffer[0], buffer.size());
    os << buffer.data();
#else
    os << str.m_data;
#endif
    return os;
}