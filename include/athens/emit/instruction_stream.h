#ifndef INSTRUCTION_STREAM_H
#define INSTRUCTION_STREAM_H

#include "instruction.h"

#include <vector>
#include <ostream>

class InstructionStream {
    friend std::ostream &operator<<(std::ostream &os, const InstructionStream &instruction_stream);
public:
    InstructionStream();
    InstructionStream(const InstructionStream &other);

    inline size_t GetPosition() const { return m_position; }

    InstructionStream &operator<<(const Instruction<> &instruction);

private:
    size_t m_position;
    std::vector<Instruction<>> m_data;
};

#endif