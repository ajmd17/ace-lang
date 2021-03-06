#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP

#include <common/utf8.hpp>

namespace ace {
namespace vm {

class Exception {
public:
    Exception(const char *str);
    Exception(const Exception &other);
    ~Exception();

    inline const char *ToString() const { return m_str; }

    static Exception InvalidComparisonException(const char *left_type_str, const char *right_type_str);
    static Exception InvalidOperationException(const char *op_name,
        const char *left_type_str, const char *right_type_str);
    static Exception InvalidOperationException(const char *op_name, const char *type_str);
    static Exception InvalidArgsException(int expected, int received, bool variadic = false);
    static Exception InvalidArgsException(const char *expected_str, int received);
    static Exception NullReferenceException();
    static Exception DivisionByZeroException();
    static Exception OutOfBoundsException();
    static Exception MemberNotFoundException();
    static Exception FileOpenException(const char *file_name);
    static Exception UnopenedFileWriteException();
    static Exception UnopenedFileReadException();
    static Exception UnopenedFileCloseException();
    static Exception LibraryLoadException(const char *lib_name);
    static Exception LibraryFunctionLoadException(const char *func_name);

private:
    char *m_str;
};

} // namespace vm
} // namespace ace

#endif
