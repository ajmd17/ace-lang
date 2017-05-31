#include <ace-vm/Exception.hpp>

namespace ace {
namespace vm {

Exception::Exception(const utf::Utf8String &str)
    : m_str(str)
{
}

Exception::Exception(const Exception &other)
    : m_str(other.m_str)
{
}

Exception Exception::InvalidComparisonException(const char *left_type_str, const char *right_type_str)
{
    char buffer[256];
    std::snprintf(
        buffer,
        255,
        "Cannot compare %s with %s",
        left_type_str,
        right_type_str
    );
    return Exception(utf::Utf8String(buffer));
}

Exception Exception::InvalidOperationException(const char *op_name, const char *left_type_str, const char *right_type_str)
{
    char buffer[256];
    std::snprintf(
        buffer,
        255,
        "Invalid operation (%s) on types %s and %s",
        op_name,
        left_type_str,
        right_type_str
    );
    return Exception(utf::Utf8String(buffer));
}

Exception Exception::InvalidOperationException(const char *op_name, const char *type_str)
{
    char buffer[256];
    std::snprintf(
        buffer,
        255,
        "Invalid operation (%s) on type %s",
        op_name,
        type_str
    );
    return Exception(utf::Utf8String(buffer));
}

Exception Exception::InvalidArgsException(int expected, int received, bool variadic)
{
    char buffer[256];
    if (variadic) {
        std::sprintf(buffer, "Invalid arguments: expected at least %d, received %d", expected, received);
    } else {
        std::sprintf(buffer, "Invalid arguments: expected %d, received %d", expected, received);
    }
    return Exception(utf::Utf8String(buffer));
}

Exception Exception::InvalidArgsException(const char *expected_str, int received)
{
    char buffer[256];
    std::sprintf(buffer, "Invalid arguments: expected %s, received %d", expected_str, received);
    return Exception(utf::Utf8String(buffer));
}

Exception Exception::NullReferenceException()
{
    return Exception(utf::Utf8String("Null reference exception"));
}

Exception Exception::DivisionByZeroException()
{
    return Exception(utf::Utf8String("Division by zero"));
}

Exception Exception::OutOfBoundsException()
{
    return Exception(utf::Utf8String("Index out of bounds of Array"));
}

Exception Exception::MemberNotFoundException()
{
    return Exception(utf::Utf8String("Member not found"));
}

Exception Exception::FileOpenException(const char *file_name)
{
    return Exception(utf::Utf8String("Failed to open file `") + file_name + "`");
}

Exception Exception::UnopenedFileWriteException()
{
    return Exception(utf::Utf8String("Attempted to write to an unopened file"));
}

Exception Exception::UnopenedFileReadException()
{
    return Exception(utf::Utf8String("Attempted to read from an unopened file"));
}

Exception Exception::UnopenedFileCloseException()
{
    return Exception(utf::Utf8String("Attempted to close an unopened file"));
}

Exception Exception::LibraryLoadException(const char *lib_name)
{
    return Exception(utf::Utf8String("Failed to load library `") + lib_name + "`");
}

Exception Exception::LibraryFunctionLoadException(const char *func_name)
{
    return Exception(utf::Utf8String("Failed to load library function `") + func_name + "`");
}

} // namespace vm
} // namespace ace