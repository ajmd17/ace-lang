#include <ace-vm/exception.hpp>

Exception::Exception(const utf::Utf8String &str)
    : m_str(str)
{
}

Exception::Exception(const Exception &other)
    : m_str(other.m_str)
{
}

Exception Exception::InvalidArgsException(int expected, int received)
{
    char buffer[256];
    std::sprintf(buffer, "invalid arguments: expected %d, received %d", expected, received);
    return Exception(utf::Utf8String(buffer));
}

Exception Exception::NullReferenceException()
{
    return Exception(utf::Utf8String("null reference exception"));
}

Exception Exception::DivisionByZeroException()
{
    return Exception(utf::Utf8String("attempted to divide by zero"));
}