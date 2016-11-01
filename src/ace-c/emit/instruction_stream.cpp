#include <ace-c/emit/instruction_stream.hpp>

#include <common/instructions.hpp>

#include <algorithm>
#include <cstring>

std::ostream &operator<<(std::ostream &os, InstructionStream instruction_stream)
{
    // create instructions to store static objects

    // sort all static objects by their id
    std::sort(instruction_stream.m_static_objects.begin(), instruction_stream.m_static_objects.end(),
        [](const StaticObject &a, const StaticObject &b)
        {
            return a.m_id < b.m_id;
        });

    // calculate label offset
    // for each label, it increases by the size of the opcode,
    // plus the size of the address
    uint32_t label_offset = (uint32_t)os.tellp();
    for (const StaticObject &so : instruction_stream.m_static_objects) {
        label_offset += sizeof(uint8_t);  // opcode
        if (so.m_type == StaticObject::TYPE_LABEL) {
            label_offset += sizeof(uint32_t); // address
        } else if (so.m_type == StaticObject::TYPE_STRING) {
            label_offset += sizeof(uint32_t); // string length
            label_offset += std::strlen(so.m_value.str);
        } else if (so.m_type == StaticObject::TYPE_FUNCTION) {
            label_offset += sizeof(uint32_t); // address
            label_offset += sizeof(uint8_t); // num args
        } else if (so.m_type == StaticObject::TYPE_TYPE_INFO) {
            label_offset += sizeof(uint8_t); // type size
            // ignore type name for now
        }
    }

    for (const StaticObject &so : instruction_stream.m_static_objects) {
        if (so.m_type == StaticObject::TYPE_LABEL) {
            Instruction<uint8_t, uint32_t> store_ins(STORE_STATIC_ADDRESS,
                so.m_value.lbl + label_offset);

            for (int i = store_ins.m_data.size() - 1; i >= 0; i--) {
                os.write(&store_ins.m_data[i][0], store_ins.m_data[i].size());
            }

        } else if (so.m_type == StaticObject::TYPE_STRING) {
            Instruction<uint8_t, uint32_t, const char*> store_ins(STORE_STATIC_STRING,
                std::strlen(so.m_value.str), so.m_value.str);

            for (int i = store_ins.m_data.size() - 1; i >= 0; i--) {
                os.write(&store_ins.m_data[i][0], store_ins.m_data[i].size());
            }
        } else if (so.m_type == StaticObject::TYPE_FUNCTION) {
            Instruction<uint8_t, uint32_t, uint8_t> store_ins(STORE_STATIC_FUNCTION,
                so.m_value.func.m_addr + label_offset, so.m_value.func.m_nargs);

            for (int i = store_ins.m_data.size() - 1; i >= 0; i--) {
                os.write(&store_ins.m_data[i][0], store_ins.m_data[i].size());
            }
        } else if (so.m_type == StaticObject::TYPE_TYPE_INFO) {
            Instruction<uint8_t, uint8_t> store_ins(STORE_STATIC_TYPE, so.m_value.type_info.m_size);
            for (int i = store_ins.m_data.size() - 1; i >= 0; i--) {
                os.write(&store_ins.m_data[i][0], store_ins.m_data[i].size());
            }
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
      m_static_id(0)
{
}

InstructionStream::InstructionStream(const InstructionStream &other)
    : m_position(other.m_position),
      m_data(other.m_data),
      m_register_counter(other.m_register_counter),
      m_stack_size(other.m_stack_size),
      m_static_id(other.m_static_id),
      m_static_objects(other.m_static_objects)
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
