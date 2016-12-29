#ifndef DECOMPILATION_UNIT_HPP
#define DECOMPILATION_UNIT_HPP

#include <ace-c/dis/ByteStream.hpp>
#include <ace-c/emit/InstructionStream.hpp>

#include <common/utf8.hpp>

class DecompilationUnit {
public:
    DecompilationUnit(const ByteStream &bs);
    DecompilationUnit(const DecompilationUnit &other) = delete;

    InstructionStream Decompile(utf::utf8_ostream *os = nullptr);

private:
    ByteStream m_bs;
};

#endif
