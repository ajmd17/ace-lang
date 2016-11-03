#ifndef ACE_C_HPP
#define ACE_C_HPP

#include <common/utf8.hpp>

namespace ace_compiler {
void BuildSourceFile(const utf::Utf8String &filename,
    const utf::Utf8String &out_filename);
bool BuildSourceString(const utf::Utf8String &code,
    const utf::Utf8String &out_filename);
void DecompileBytecodeFile(const utf::Utf8String &filename,
    const utf::Utf8String &out_filename);
} // namespace ace_compiler

#endif
