#include <athens/emit/instruction_stream.h>

#include <common/instructions.h>

std::ostream &operator<<(std::ostream &os, InstructionStream instruction_stream)
{
    // create instructions to store static objects
    // handle strings
    for (const StaticString &ss : instruction_stream.m_static_strings) {
        //STORE_STATIC_STRING
        Instruction<uint8_t, uint32_t, const char*> store_ins(
                STORE_STATIC_STRING, ss.m_value.length(), ss.m_value.c_str());

        for (int i = store_ins.m_data.size() - 1; i >= 0; i--) {
            os.write(&store_ins.m_data[i][0], store_ins.m_data[i].size());
        }
    }

    for (const Instruction<> &ins : instruction_stream.m_data) {
        for (int i = ins.m_data.size() - 1; i >= 0; i--) {
            os.write(&ins.m_data[i][0], ins.m_data[i].size());
        }
    }
    return os;
}

InstructionStream::InstructionStream()
    : m_position(0),
      m_register_counter(0),
      m_stack_size(0),
      m_label_id(0),
      m_static_id(0)
{
}

InstructionStream::InstructionStream(const InstructionStream &other)
    : m_position(other.m_position), 
      m_data(other.m_data),
      m_register_counter(other.m_register_counter),
      m_stack_size(other.m_stack_size),
      m_label_id(other.m_label_id),
      m_static_id(other.m_static_id),
      m_static_strings(other.m_static_strings)
{
}

InstructionStream &InstructionStream::operator<<(const Instruction<> &instruction)
{
    m_data.push_back(instruction);
    for (const std::vector<char> &operand : instruction.m_data) {
        m_position += operand.size();
    }
    return *this;
}
