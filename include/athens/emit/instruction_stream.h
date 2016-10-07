#ifndef INSTRUCTION_STREAM_H
#define INSTRUCTION_STREAM_H

#include "instruction.h"

#include <vector>
#include <ostream>
#include <cstdint>

class InstructionStream {
    friend std::ostream &operator<<(std::ostream &os, const InstructionStream &instruction_stream);
public:
    InstructionStream();
    InstructionStream(const InstructionStream &other);

    inline size_t GetPosition() const { return m_position; }
    inline uint8_t GetCurrentRegister() const { return m_register_counter; }
    inline void IncRegisterUsage() { m_register_counter++; }
    inline void DecRegisterUsage() { m_register_counter--; }
    inline int GetStackSize() const { return m_stack_size; }
    inline void IncStackSize() { m_stack_size++; }
    inline void DecStackSize() { m_stack_size--; }

    InstructionStream &operator<<(const Instruction<> &instruction);


private:
    size_t m_position;
    std::vector<Instruction<>> m_data;
    // incremented and decremented each time a register
    // is used/unused
    uint8_t m_register_counter;
    // incremented each time a variable is pushed,
    // decremented each time a stack frame is closed
    int m_stack_size;
};

#endif