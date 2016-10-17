#ifndef DECOMPILATION_UNIT_H
#define DECOMPILATION_UNIT_H

#include <athens/dis/byte_stream.h>
#include <athens/emit/instruction_stream.h>

#include <ostream>

class DecompilationUnit {
public:
    DecompilationUnit(const ByteStream &bs);
    DecompilationUnit(const DecompilationUnit &other) = delete;

    InstructionStream Decompile(std::ostream *os = nullptr);

private:
    ByteStream m_bs;
};

#endif