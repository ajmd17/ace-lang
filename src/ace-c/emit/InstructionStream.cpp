#include <ace-c/emit/InstructionStream.hpp>
#include <ace-c/Configuration.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

#include <algorithm>
#include <iostream>
#include <cstring>

std::ostream &operator<<(std::ostream &os, InstructionStream instruction_stream)
{
    // sort all static objects by their id
    std::sort(
        instruction_stream.m_static_objects.begin(),
        instruction_stream.m_static_objects.end(),
        [](const StaticObject &a, const StaticObject &b) {
            return a.m_id < b.m_id;
        }
    );

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
                label_offset += sizeof(uint16_t); // size of name
                label_offset += std::strlen(so.m_value.type_info.m_name);

                label_offset += sizeof(uint16_t); // type size
                // names
                for (size_t i = 0; i < so.m_value.type_info.m_names.size(); i++) {
                    label_offset += sizeof(so.m_value.type_info.m_names[i].first);
                    label_offset += so.m_value.type_info.m_names[i].second.size();
                }
            }
        }

        // create instructions to store static objects
        for (const StaticObject &so : instruction_stream.m_static_objects) {
            if (so.m_type == StaticObject::TYPE_LABEL) {
                Instruction<uint8_t, uint32_t> store_ins(
                    STORE_STATIC_ADDRESS,
                    so.m_value.lbl + label_offset
                );

                for (auto it = store_ins.m_data.rbegin(); it != store_ins.m_data.rend(); ++it) {
                    os.write(&(*it)[0], it->size());
                }
            } else if (so.m_type == StaticObject::TYPE_STRING) {
                Instruction<uint8_t, uint32_t, const char*> store_ins(
                    STORE_STATIC_STRING,
                    std::strlen(so.m_value.str),
                    so.m_value.str
                );

                for (auto it = store_ins.m_data.rbegin(); it != store_ins.m_data.rend(); ++it) {
                    if (!it->empty()) {
                        os.write(&(*it)[0], it->size());
                    }
                }
            } else if (so.m_type == StaticObject::TYPE_FUNCTION) {
                Instruction<uint8_t, uint32_t, uint8_t, uint8_t> store_ins(
                    STORE_STATIC_FUNCTION,
                    so.m_value.func.m_addr + label_offset,
                    so.m_value.func.m_nargs,
                    so.m_value.func.m_flags
                );

                for (auto it = store_ins.m_data.rbegin(); it != store_ins.m_data.rend(); ++it) {
                    os.write(&(*it)[0], it->size());
                }
            } else if (so.m_type == StaticObject::TYPE_TYPE_INFO) {
                const size_t len = std::strlen(so.m_value.type_info.m_name);

                Instruction<uint8_t, NamesPair_t, uint16_t, std::vector<NamesPair_t>> store_ins(
                    STORE_STATIC_TYPE,
                    {
                        len,
                        std::vector<uint8_t>(so.m_value.type_info.m_name, &so.m_value.type_info.m_name[len])
                    },
                    so.m_value.type_info.m_size,
                    so.m_value.type_info.m_names
                );

                for (auto it = store_ins.m_data.rbegin(); it != store_ins.m_data.rend(); ++it) {
                    os.write(&(*it)[0], it->size());
                }
            }
        }
    } else {
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
                        LOAD_ADDR,
                        reg,
                        so.m_value.lbl + label_offset
                    );
                    ins = tmp;
                } else if (so.m_type == StaticObject::TYPE_STRING) {
                    Instruction<> tmp = Instruction<uint8_t, uint8_t, uint32_t, const char*>(
                        LOAD_STRING,
                        reg,
                        std::strlen(so.m_value.str),
                        so.m_value.str
                    );
                    ins = tmp;
                } else if (so.m_type == StaticObject::TYPE_FUNCTION) {
                    Instruction<> tmp = Instruction<uint8_t, uint8_t, uint32_t, uint8_t, uint8_t>(
                        LOAD_FUNC,
                        reg,
                        so.m_value.func.m_addr + label_offset,
                        so.m_value.func.m_nargs, 
                        so.m_value.func.m_flags
                    );
                    ins = tmp;
                } else if (so.m_type == StaticObject::TYPE_TYPE_INFO) {
                    size_t len = std::strlen(so.m_value.type_info.m_name);

                    Instruction<> tmp = Instruction<uint8_t, uint8_t, NamesPair_t, uint16_t, std::vector<NamesPair_t>>(
                        LOAD_TYPE,
                        reg,
                        {
                            len,
                            std::vector<uint8_t>(so.m_value.type_info.m_name, &so.m_value.type_info.m_name[len])
                        },
                        so.m_value.type_info.m_size,
                        so.m_value.type_info.m_names
                    );
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

size_t InstructionStream::Allot(const Instruction<> &instruction)
{
    size_t sz = 0;

    for (const std::vector<char> &operand : instruction.m_data) {
        sz += operand.size();
    }

    m_position += sz;

    size_t index = m_instruction_block.m_allotted.size();
    m_instruction_block.m_allotted.push_back(sz);

    return index;
}

bool InstructionStream::Write(size_t allotted_index, const Instruction<> &instruction)
{
    if (allotted_index >= m_instruction_block.m_allotted.size()) {
        return false;
    }

    size_t sz = 0;
    for (const std::vector<char> &operand : instruction.m_data) {
        sz += operand.size();
    }

    if (m_instruction_block.m_allotted[allotted_index] != sz) {
        return false;
    }

    m_data.push_back(instruction);

    return true;
}

InstructionStream &InstructionStream::operator<<(const Instruction<> &instruction)
{
    m_data.push_back(instruction);
    for (const std::vector<char> &operand : instruction.m_data) {
        m_position += operand.size();
    }
    return *this;
}
