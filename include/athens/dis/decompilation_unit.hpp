#ifndef DECOMPILATION_UNIT_HPP
#define DECOMPILATION_UNIT_HPP

#include <athens/dis/byte_stream.hpp>
#include <athens/emit/instruction_stream.hpp>

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
