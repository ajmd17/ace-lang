#ifndef ACE_C_HPP
#define ACE_C_HPP

#include <ace-c/CompilationUnit.hpp>
#include <ace-c/emit/BytecodeChunk.hpp>

#include <common/utf8.hpp>

#include <memory>

namespace ace_compiler {

std::unique_ptr<BytecodeChunk> BuildSourceFile(
    const utf::Utf8String &filename,
    const utf::Utf8String &out_filename);

std::unique_ptr<BytecodeChunk> BuildSourceFile(
    const utf::Utf8String &filename,
    const utf::Utf8String &out_filename, CompilationUnit &compilation_unit);

void DecompileBytecodeFile(const utf::Utf8String &filename,
    const utf::Utf8String &out_filename);

} // ace_compiler

#endif
