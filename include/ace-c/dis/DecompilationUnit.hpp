#ifndef DECOMPILATION_UNIT_HPP
#define DECOMPILATION_UNIT_HPP

#include <ace-vm/BytecodeStream.hpp>
#include <ace-c/emit/InstructionStream.hpp>

#include <common/utf8.hpp>

class DecompilationUnit {
public:
    DecompilationUnit();
    DecompilationUnit(const DecompilationUnit &other) = delete;

    void DecodeNext(uint8_t code, ace::vm::BytecodeStream &bs, InstructionStream &is, utf::utf8_ostream *os = nullptr);
    InstructionStream Decompile(ace::vm::BytecodeStream &bs, utf::utf8_ostream *os = nullptr);
};

#endif
