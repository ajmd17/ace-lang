#include <athens/emit/instruction_stream.h>

std::ostream &operator<<(std::ostream &os, const InstructionStream &instruction_stream)
{
    for (const Instruction<> &ins : instruction_stream.m_data) {
        int index = ins.m_data.size() - 1;
        while (index >= 0) {
            os.write(&ins.m_data[index][0], ins.m_data[index].size());
            --index;
        }
    }
    return os;
}

InstructionStream::InstructionStream()
    : m_position(0)
{
}

InstructionStream::InstructionStream(const InstructionStream &other)
    : m_position(other.m_position), 
      m_data(other.m_data)
{
}

InstructionStream &InstructionStream::operator<<(const Instruction<> &instruction)
{
    m_data.push_back(instruction);
    for (const std::vector<char> &operand : instruction.m_data) {
        for (size_t i = 0; i < operand.size(); i++, m_position++);
    }
    return *this;
}
