#ifndef DECOMPILATION_UNIT_HPP
#define DECOMPILATION_UNIT_HPP

#include <ace-c/dis/byte_stream.hpp>
#include <ace-c/emit/instruction_stream.hpp>

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
