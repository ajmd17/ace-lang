#ifndef ACE_C_HPP
#define ACE_C_HPP

#include <ace-c/compilation_unit.hpp>
#include <common/utf8.hpp>

namespace ace_compiler {

bool BuildSourceFile(const utf::Utf8String &filename,
    const utf::Utf8String &out_filename);

bool BuildSourceFile(const utf::Utf8String &filename,
    const utf::Utf8String &out_filename, CompilationUnit &compilation_unit);

void DecompileBytecodeFile(const utf::Utf8String &filename,
    const utf::Utf8String &out_filename);

} // ace_compiler

#endif
