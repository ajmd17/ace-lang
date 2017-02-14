#include <ace-c/emit/InstructionStream.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

#include <algorithm>
#include <cstring>

std::ostream &operator<<(std::ostream &os, InstructionStream instruction_stream)
{
    // sort all static objects by their id
    std::sort(instruction_stream.m_static_objects.begin(), instruction_stream.m_static_objects.end(),
        [](const StaticObject &a, const StaticObject &b) {
            return a.m_id < b.m_id;
        });

    int label_offset = (int)os.tellp();

    if (ace::compiler::Config::use_static_objects) {
        // calculate label offset
        // for each label, it increases by the size of the opcode,
        // plus the size of the address
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
                label_offset += sizeof(uint8_t); // is variadic
            } else if (so.m_type == StaticObject::TYPE_TYPE_INFO) {
                label_offset += sizeof(uint16_t); // type size
                label_offset += so.m_value.type_info.m_size * sizeof(uint32_t); // size of hashes
            }
        }

        // create instructions to store static objects
        for (const StaticObject &so : instruction_stream.m_static_objects) {
            if (so.m_type == StaticObject::TYPE_LABEL) {
                Instruction<uint8_t, uint32_t> store_ins(STORE_STATIC_ADDRESS,
                    so.m_value.lbl + label_offset);

                for (auto it = store_ins.m_data.rbegin(); it != store_ins.m_data.rend(); ++it) {
                    os.write(&(*it)[0], it->size());
                }
            } else if (so.m_type == StaticObject::TYPE_STRING) {
                Instruction<uint8_t, uint32_t, const char*> store_ins(STORE_STATIC_STRING,
                    std::strlen(so.m_value.str), so.m_value.str);

                for (auto it = store_ins.m_data.rbegin(); it != store_ins.m_data.rend(); ++it) {
                    if (!it->empty()) os.write(&(*it)[0], it->size());
                }
            } else if (so.m_type == StaticObject::TYPE_FUNCTION) {
                Instruction<uint8_t, uint32_t, uint8_t, uint8_t> store_ins(STORE_STATIC_FUNCTION,
                    so.m_value.func.m_addr + label_offset, so.m_value.func.m_nargs, so.m_value.func.m_is_variadic);

                for (auto it = store_ins.m_data.rbegin(); it != store_ins.m_data.rend(); ++it) {
                    os.write(&(*it)[0], it->size());
                }
            } else if (so.m_type == StaticObject::TYPE_TYPE_INFO) {
                Instruction<uint8_t, uint16_t, std::vector<uint32_t>> store_ins(
                    STORE_STATIC_TYPE, so.m_value.type_info.m_size, so.m_value.type_info.m_hashes);

                for (auto it = store_ins.m_data.rbegin(); it != store_ins.m_data.rend(); ++it) {
                    os.write(&(*it)[0], it->size());
                }
            }
        }
    } else {
        // accumulate label offset
        /*for (const Instruction<> &ins : instruction_stream.m_data) {
            if (ins.GetOpcode() == LOAD_STATIC) {
                std::vector<char> idx_data = ins.m_data.at(0);
                std::vector<char> reg_data = ins.m_data.at(1);

                uint16_t idx = *reinterpret_cast<uint16_t*>(idx_data.data());
                uint8_t reg = *reinterpret_cast<uint8_t*>(reg_data.data());

                const StaticObject &so = instruction_stream.m_static_objects.at(idx);

                size_t size_before = 0;
                for (const auto &vec : ins.m_data) {
                    size_before += vec.size();
                }

                size_t size_after = 0;
                if (so.m_type == StaticObject::TYPE_LABEL) {
                    // LOAD_ADDR size
                    size_after += sizeof(uint8_t);
                    size_after += sizeof(uint8_t);
                    size_after += sizeof(uint32_t);
                } else if (so.m_type == StaticObject::TYPE_STRING) {
                    // LOAD_STRING size
                    size_after += sizeof(uint8_t);
                    size_after += sizeof(uint8_t);
                    size_after += sizeof(uint32_t);
                    size_after += std::strlen(so.m_value.str);
                } else if (so.m_type == StaticObject::TYPE_FUNCTION) {
                    // LOAD_FUNC size
                    size_after += sizeof(uint8_t);
                    size_after += sizeof(uint8_t);
                    size_after += sizeof(uint32_t);
                    size_after += sizeof(uint8_t);
                } else if (so.m_type == StaticObject::TYPE_TYPE_INFO) {
                    // LOAD_TYPE size
                    size_after += sizeof(uint8_t);
                    size_after += sizeof(uint8_t);
                    size_after += sizeof(uint8_t);
                }

                int diff = size_after - size_before;
                label_offset += diff;
            }
        }*/

        // static objects are turned off.
        // convert all LOAD_STATIC instructions.
        for (Instruction<> &ins : instruction_stream.m_data) {
            if (ins.GetOpcode() == LOAD_STATIC) {
                ASSERT(ins.m_data.size() >= 2);

                std::vector<char> idx_data = ins.m_data[0];
                std::vector<char> reg_data = ins.m_data[1];

                uint16_t idx = *reinterpret_cast<uint16_t*>(idx_data.data());
                uint8_t reg = *reinterpret_cast<uint8_t*>(reg_data.data());

                const StaticObject &so = instruction_stream.m_static_objects.at(idx);

                if (so.m_type == StaticObject::TYPE_LABEL) {
                    Instruction<> tmp = Instruction<uint8_t, uint8_t, uint32_t>(
                        LOAD_ADDR, reg, so.m_value.lbl + label_offset);
                    ins = tmp;
                } else if (so.m_type == StaticObject::TYPE_STRING) {
                    Instruction<> tmp = Instruction<uint8_t, uint8_t, uint32_t, const char*>(
                        LOAD_STRING, reg, std::strlen(so.m_value.str), so.m_value.str);
                    ins = tmp;
                } else if (so.m_type == StaticObject::TYPE_FUNCTION) {
                    Instruction<> tmp = Instruction<uint8_t, uint8_t, uint32_t, uint8_t, uint8_t>(
                        LOAD_FUNC, reg, so.m_value.func.m_addr + label_offset, so.m_value.func.m_nargs, 
                            so.m_value.func.m_is_variadic);
                    ins = tmp;
                } else if (so.m_type == StaticObject::TYPE_TYPE_INFO) {
                    Instruction<> tmp = Instruction<uint8_t, uint8_t, uint16_t, std::vector<uint32_t>>(
                        LOAD_TYPE, reg, so.m_value.type_info.m_size, so.m_value.type_info.m_hashes);
                    ins = tmp;
                }
            }
        }
    }

    for (const Instruction<> &ins : instruction_stream.m_data) {
        for (int i = ins.m_data.size() - 1; i >= 0; i--) {
            if (!ins.m_data[i].empty()) {
                os.write(&ins.m_data[i][0], ins.m_data[i].size());
            }
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
