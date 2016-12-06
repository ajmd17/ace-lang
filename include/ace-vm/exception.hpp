#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP

#include <common/utf8.hpp>

class Exception {
public:
    Exception(const utf::Utf8String &str);
    Exception(const Exception &other);

    inline const utf::Utf8String &ToString() const { return m_str; }

    static Exception InvalidArgsException(int expected, int received);
    static Exception NullReferenceException();
    static Exception DivisionByZeroException();
    static Exception OutOfBoundsException();

private:
    const utf::Utf8String m_str;
};

#endif
