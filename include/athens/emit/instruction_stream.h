#ifndef INSTRUCTION_STREAM_H
#define INSTRUCTION_STREAM_H

#include <athens/emit/instruction.h>
#include <athens/emit/label.h>
#include <athens/emit/static_object.h>

#include <vector>
#include <ostream>
#include <cstdint>

class InstructionStream {
    friend std::ostream &operator<<(std::ostream &os, InstructionStream instruction_stream);
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

    inline int NewLabelId() { return m_label_id++; }
    inline void AddLabel(const Label &label) { m_labels.push_back(label); }

    inline int NewStaticId() { return m_static_id++; }
    inline void AddStaticString(const StaticString &static_string) { m_static_strings.push_back(static_string); }

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
    // all labels in the bytecode stream
    std::vector<Label> m_labels;
    // the current label id
    int m_label_id;
    // the current static object id
    int m_static_id;
    // all static strings
    std::vector<StaticString> m_static_strings;
};

#endif