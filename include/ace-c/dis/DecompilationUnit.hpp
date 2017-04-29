#ifndef DECOMPILATION_UNIT_HPP
#define DECOMPILATION_UNIT_HPP

#include <ace-c/dis/ByteStream.hpp>
#include <ace-c/emit/InstructionStream.hpp>

#include <common/utf8.hpp>

class DecompilationUnit {
public:
    DecompilationUnit();
    DecompilationUnit(const DecompilationUnit &other) = delete;

    void DecodeNext(ByteStream &bs, InstructionStream &is, utf::utf8_ostream *os = nullptr);
    InstructionStream Decompile(ByteStream &bs, utf::utf8_ostream *os = nullptr);
};

#endif
