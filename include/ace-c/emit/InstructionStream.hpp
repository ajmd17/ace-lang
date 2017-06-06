#ifndef INSTRUCTION_STREAM_HPP
#define INSTRUCTION_STREAM_HPP

#include <ace-c/emit/Instruction.hpp>
#include <ace-c/emit/StaticObject.hpp>

#include <vector>
#include <ostream>
#include <cstdint>

/*struct BytecodeLabel {
    int id;
};

class BytecodeOutputStream {
public:
    static unsigned char EncodeOpcode(unsigned char opcode, unsigned char data);

    void Add_NOP();

    void Add_STORE(const std::string &str);
    void Add_STORE(uint32_t addr);
    void Add_STORE(uint32_t addr, uint32_t nargs);
    void Add_STORE(const std::string &type_name, const std::vector<uint32_t> &hashes);

    void Add_CONST_I32(int32_t i);
    void Add_CONST_I64(int64_t i);
    void Add_CONST_F32(float f);
    void Add_CONST_F64(double d);
    void Add_CONST_BOOLEAN(bool b);
    void Add_CONST_NULL();

    void Add_MOV_REG(MovRegMode mode, int a, int b);
    void Add_MOV_INDEX(MovIndexMode mode, int a, int b);

    void Add_JMP(BytecodeLabel label);
    void Add_JMP_IF(JumpMode mode, BytecodeLabel label);
    

private:
    std::vector<std::vector<unsigned char>> m_data;
};*/

class InstructionStream {
    friend std::ostream &operator<<(std::ostream &os, InstructionStream instruction_stream);
    
public:
    InstructionStream();
    InstructionStream(const InstructionStream &other);

    inline void ClearInstructions() { m_position = 0; m_data.clear(); }

    inline size_t GetPosition() const { return m_position; }
    inline size_t &GetPosition() { return m_position; }

    inline const std::vector<Instruction<>> &GetData() const { return m_data; }

    inline uint8_t GetCurrentRegister() const { return m_register_counter; }
    inline uint8_t IncRegisterUsage() { return ++m_register_counter; }
    inline uint8_t DecRegisterUsage() { return --m_register_counter; }

    inline int GetStackSize() const { return m_stack_size; }
    inline int IncStackSize() { return ++m_stack_size; }
    inline int DecStackSize() { return --m_stack_size; }

    inline int NewStaticId() { return m_static_id++; }

    inline void AddStaticObject(const StaticObject &static_object) { m_static_objects.push_back(static_object); }

    inline int FindStaticObject(const StaticObject &static_object) const
    {
        for (const StaticObject &so : m_static_objects) {
            if (so == static_object) {
                return so.m_id;
            }
        }
        // not found
        return -1;
    }

    inline void BeginBlock()
    {
        m_instruction_block.m_allotted.clear();
    }

    inline void EndBlock()
    {
        m_instruction_block.m_allotted.clear();
    }

    size_t Allot(const Instruction<> &instruction);
    bool Write(size_t allotted_index, const Instruction<> &instruction);

    InstructionStream &operator<<(const Instruction<> &instruction);

private:
    struct {
        std::vector<size_t> m_allotted;
    } m_instruction_block;

    size_t m_position;
    std::vector<Instruction<>> m_data;
    // incremented and decremented each time a register
    // is used/unused
    uint8_t m_register_counter;
    // incremented each time a variable is pushed,
    // decremented each time a stack frame is closed
    int m_stack_size;
    // the current static object id
    int m_static_id;

    std::vector<StaticObject> m_static_objects;
};

#endif
