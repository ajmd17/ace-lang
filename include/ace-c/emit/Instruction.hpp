#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include <ace-c/emit/NamesPair.hpp>

#include <common/instructions.hpp>
#include <common/my_assert.hpp>

#define CEREAL_SERIALIZE_FUNCTION_NAME Serialize
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>

#include <ace-c/emit/Buildable.hpp>

#include <vector>
#include <ostream>
#include <cstring>
#include <cstdint>
#include <iostream>

using Opcode = uint8_t;
using RegIndex = uint8_t;
using LabelId = size_t;

struct Instruction : public Buildable {
    Opcode opcode;

    virtual ~Instruction() = default;

    virtual size_t GetSize() const override = 0;
    virtual void Build(Buffer &buf, BuildParams &build_params) const override = 0;
};

struct LabelMarker final : public Buildable {
    LabelMarker(LabelId id)
        : id(id)
    {
    }

    virtual ~LabelMarker() = default;

    virtual size_t GetSize() const override
    {
        return 0;
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
    }

    LabelId id;
};

struct Jump final : public Buildable {
    enum JumpClass {
        JMP,
        JE,
        JNE,
        JG,
        JGE,
    } jump_class;
    LabelId label_id;

    Jump() = default;
    Jump(JumpClass jump_class, LabelId label_id)
        : jump_class(jump_class),
          label_id(label_id)
    {
    }

    virtual ~Jump() = default;

    virtual size_t GetSize() const override
    {
        return sizeof(Opcode) + sizeof(LabelPosition);
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        switch (jump_class) {
            case JumpClass::JMP:
                buf.sputc(Instructions::JMP);
                break;
            case JumpClass::JE:
                buf.sputc(Instructions::JE);
                break;
            case JumpClass::JNE:
                buf.sputc(Instructions::JNE);
                break;
            case JumpClass::JG:
                buf.sputc(Instructions::JG);
                break;
            case JumpClass::JGE:
                buf.sputc(Instructions::JGE);
                break;
        }

        LabelPosition pos = build_params.block_offset
            + build_params.labels[label_id].position;

        buf.sputn((byte*)&pos, sizeof(pos));
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(jump_class), CEREAL_NVP(label_id));
    }
};

struct Comparison final : public Buildable {
    enum ComparisonClass {
        CMP,
        CMPZ
    } comparison_class;

    RegIndex reg_lhs;
    RegIndex reg_rhs;

    Comparison() = default;

    Comparison(ComparisonClass comparison_class, RegIndex reg)
        : comparison_class(comparison_class),
          reg_lhs(reg)
    {
    }

    Comparison(ComparisonClass comparison_class, RegIndex reg_lhs, RegIndex reg_rhs)
        : comparison_class(comparison_class),
          reg_lhs(reg_lhs),
          reg_rhs(reg_rhs)
    {
    }

    virtual ~Comparison() = default;

    virtual size_t GetSize() const override
    {
        size_t sz = sizeof(Opcode)
            + sizeof(reg_lhs);

        if (comparison_class == ComparisonClass::CMP) {
            sz += sizeof(reg_rhs);
        }

        return sz;
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        switch (comparison_class) {
            case ComparisonClass::CMP:
                buf.sputc(Instructions::CMP);
                break;
            case ComparisonClass::CMPZ:
                buf.sputc(Instructions::CMPZ);
                break;
        }

        buf.sputc(reg_lhs);

        if (comparison_class == ComparisonClass::CMP) {
            buf.sputc(reg_rhs);
        }
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(comparison_class), CEREAL_NVP(reg_lhs), CEREAL_NVP(reg_rhs));
    }
};

struct FunctionCall : public Buildable {
    RegIndex reg;
    uint8_t nargs;

    FunctionCall() = default;
    FunctionCall(RegIndex reg, uint8_t nargs)
        : reg(reg),
          nargs(nargs)
    {
    }

    virtual ~FunctionCall() = default;

    virtual size_t GetSize() const override
    {
        return sizeof(Opcode)
            + sizeof(reg)
            + sizeof(nargs);
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        buf.sputc(Instructions::CALL);
        buf.sputc(reg);
        buf.sputc(nargs);
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(reg), CEREAL_NVP(nargs));
    }
};

struct Return : public Buildable {
    Return() = default;
    virtual ~Return() = default;

    virtual size_t GetSize() const override
    {
        return sizeof(Opcode);
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        buf.sputc(Instructions::RET);
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
    }
};

struct StoreLocal : public Buildable {
    StoreLocal() = default;
    StoreLocal(RegIndex reg)
        : reg(reg)
    {
    }
    virtual ~StoreLocal() = default;

    virtual size_t GetSize() const override
    {
        return sizeof(Opcode)
            + sizeof(reg);
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        buf.sputc(Instructions::PUSH);
        buf.sputc(reg);
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(reg));
    }

    RegIndex reg;
};

struct PopLocal : public Buildable {
    PopLocal() = default;
    PopLocal(size_t amt)
        : amt(amt)
    {
    }
    virtual ~PopLocal() = default;

    virtual size_t GetSize() const override
    {
        size_t sz = sizeof(Opcode);

        if (amt > 1) {
            sz++;
        }

        return sz;
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        if (amt > 1) {
            buf.sputc(Instructions::POP_N);

            byte as_byte = (byte)amt;
            buf.sputc(as_byte);
        } else {
            buf.sputc(Instructions::POP);
        }
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(amt));
    }

    size_t amt;
};

struct ConstI32 : public Buildable {
    RegIndex reg;
    int32_t value;

    ConstI32() = default;
    ConstI32(RegIndex reg, int32_t value)
        : reg(reg),
          value(value)
    {
    }

    virtual ~ConstI32() = default;

    virtual size_t GetSize() const override
    {
        return sizeof(Opcode)
            + sizeof(reg)
            + sizeof(value);
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        buf.sputc(Instructions::LOAD_I32);
        buf.sputc(reg);
        buf.sputn((byte*)&value, sizeof(value));
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(reg), CEREAL_NVP(value));
    }
};

struct ConstI64 : public Buildable {
    RegIndex reg;
    int64_t value;

    ConstI64() = default;
    ConstI64(RegIndex reg, int64_t value)
        : reg(reg),
          value(value)
    {
    }

    virtual ~ConstI64() = default;

    virtual size_t GetSize() const override
    {
        return sizeof(Opcode)
            + sizeof(reg)
            + sizeof(value);
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        buf.sputc(Instructions::LOAD_I64);
        buf.sputc(reg);
        buf.sputn((byte*)&value, sizeof(value));
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(reg), CEREAL_NVP(value));
    }
};

struct ConstF32 : public Buildable {
    RegIndex reg;
    float value;

    ConstF32() = default;
    ConstF32(RegIndex reg, float value)
        : reg(reg),
          value(value)
    {
    }

    virtual ~ConstF32() = default;

    virtual size_t GetSize() const override
    {
        return sizeof(Opcode)
            + sizeof(reg)
            + sizeof(value);
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        buf.sputc(Instructions::LOAD_F32);
        buf.sputc(reg);
        buf.sputn((byte*)&value, sizeof(value));
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(reg), CEREAL_NVP(value));
    }
};

struct ConstF64 : public Buildable {
    RegIndex reg;
    double value;

    ConstF64() = default;
    ConstF64(RegIndex reg, double value)
        : reg(reg),
          value(value)
    {
    }

    virtual ~ConstF64() = default;

    virtual size_t GetSize() const override
    {
        return sizeof(Opcode)
            + sizeof(reg)
            + sizeof(value);
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        buf.sputc(Instructions::LOAD_F64);
        buf.sputc(reg);
        buf.sputn((byte*)&value, sizeof(value));
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(reg), CEREAL_NVP(value));
    }
};

struct ConstBool : public Buildable {
    RegIndex reg;
    bool value;

    ConstBool() = default;
    ConstBool(RegIndex reg, bool value)
        : reg(reg),
          value(value)
    {
    }

    virtual ~ConstBool() = default;

    virtual size_t GetSize() const override
    {
        return sizeof(Opcode)
            + sizeof(reg);
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        buf.sputc(value ? Instructions::LOAD_TRUE : Instructions::LOAD_FALSE);
        buf.sputc(reg);
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(reg), CEREAL_NVP(value));
    }
};

struct ConstNull : public Buildable {
    RegIndex reg;

    ConstNull() = default;
    ConstNull(RegIndex reg)
        : reg(reg)
    {
    }

    virtual ~ConstNull() = default;

    virtual size_t GetSize() const override
    {
        return sizeof(Opcode)
            + sizeof(reg);
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        buf.sputc(Instructions::LOAD_NULL);
        buf.sputc(reg);
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(reg));
    }
};

struct BuildableTryCatch final : public Buildable {
    LabelId catch_label_id;

    virtual size_t GetSize() const override
    {
        return sizeof(Opcode) + sizeof(LabelPosition);
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        buf.sputc(Instructions::BEGIN_TRY);

        LabelPosition pos = build_params.block_offset
            + build_params.labels[catch_label_id].position;

        buf.sputn((byte*)&pos, sizeof(pos));
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(catch_label_id));
    }
};

struct BuildableFunction final : public Buildable {
    RegIndex reg;
    LabelId label_id;
    uint8_t nargs;
    uint8_t flags;

    virtual size_t GetSize() const override
    {
        return sizeof(Opcode)
            + sizeof(reg)
            + sizeof(LabelPosition)
            + sizeof(nargs)
            + sizeof(flags);
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        LabelPosition pos = build_params.block_offset
            + build_params.labels[label_id].position;

        // TODO: make it store and load statically
        buf.sputc(Instructions::LOAD_FUNC);
        buf.sputc(reg);
        buf.sputn((byte*)&pos, sizeof(pos));
        buf.sputc(nargs);
        buf.sputc(flags);
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(reg), CEREAL_NVP(label_id), CEREAL_NVP(nargs), CEREAL_NVP(flags));
    }
};

struct BuildableType final : public Buildable {
    RegIndex reg;
    std::string name;
    std::vector<std::string> members;

    virtual ~BuildableType() = default;

    virtual size_t GetSize() const override
    {
        size_t sz = sizeof(Opcode)
            + sizeof(reg)
            + sizeof(uint16_t)
            + name.length();

        for (const std::string &member_name : members) {
            sz += sizeof(uint16_t);
            sz += member_name.length();
        }

        return sz;
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        // TODO: make it store and load statically
        buf.sputc(Instructions::LOAD_TYPE);
        buf.sputc(reg);

        uint16_t name_len = (uint16_t)name.length();
        buf.sputn((byte*)&name_len, sizeof(name_len));
        buf.sputn((byte*)&name[0], name.length());

        for (const std::string &member_name : members) {
            uint16_t member_name_len = (uint16_t)member_name.length();
            buf.sputn((byte*)&member_name_len, sizeof(member_name_len));
            buf.sputn((byte*)&member_name[0], member_name.length());
        }
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(reg), CEREAL_NVP(name), CEREAL_NVP(members));
    }
};

struct BuildableString final : public Buildable {
    RegIndex reg;
    std::string value;

    virtual ~BuildableString() = default;

    virtual size_t GetSize() const override
    {
        return sizeof(Opcode)
            + sizeof(reg)
            + sizeof(uint32_t)
            + value.length();
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        uint32_t len = value.length();

        // TODO: make it store and load statically
        buf.sputc(Instructions::LOAD_STRING);
        buf.sputc(reg);
        buf.sputn((byte*)&len, sizeof(len));
        buf.sputn((byte*)&value[0], value.length());
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(reg), CEREAL_NVP(value));
    }
};

struct BinOp final : public Instruction {
    RegIndex reg_lhs;
    RegIndex reg_rhs;
    RegIndex reg_dst;

    virtual size_t GetSize() const override
    {
        return sizeof(opcode)
            + sizeof(reg_lhs)
            + sizeof(reg_rhs)
            + sizeof(reg_dst);
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        buf.sputc(opcode);
        buf.sputc(reg_lhs);
        buf.sputc(reg_rhs);
        buf.sputc(reg_dst);
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(opcode), CEREAL_NVP(reg_lhs), CEREAL_NVP(reg_rhs), CEREAL_NVP(reg_dst));
    }
};

template<class...Args>
struct RawOperation final : public Instruction {
    std::vector<char> data;

    RawOperation() = default;
    RawOperation(const RawOperation &other)
        : data(other.data)
    {
    }

    virtual size_t GetSize() const override
    {
        return sizeof(opcode) + data.size();
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        buf.sputc(opcode);
        buf.sputn((byte*)&data[0], data.size());
    }

    void Accept(const char *str)
    {
        // do not copy NUL byte
        size_t length = std::strlen(str);
        data.insert(data.end(), str, str + length);
    }

    template <typename T>
    void Accept(const std::vector<T> &ts)
    {
        for (const T &t : ts) {
            this->Accept(t);
        }
    }

    template <typename T>
    void Accept(const T &t)
    {
        char bytes[sizeof(t)];
        std::memcpy(&bytes[0], &t, sizeof(t));
        data.insert(data.end(), &bytes[0], &bytes[0] + sizeof(t));
    }

    template <class Archive>
    void Serialize(Archive &archive)
    {
        archive(CEREAL_NVP(opcode), CEREAL_NVP(data));
    }
};

template <class T, class... Ts>
struct RawOperation<T, Ts...> : RawOperation<Ts...> {
    RawOperation(T t, Ts...ts) : RawOperation<Ts...>(ts...)
        { this->Accept(t); }
};

CEREAL_REGISTER_TYPE(Jump)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, Jump)

CEREAL_REGISTER_TYPE(Comparison)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, Comparison)

CEREAL_REGISTER_TYPE(FunctionCall)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, FunctionCall)

CEREAL_REGISTER_TYPE(Return)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, Return)

CEREAL_REGISTER_TYPE(StoreLocal)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, StoreLocal)

CEREAL_REGISTER_TYPE(PopLocal)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, PopLocal)

CEREAL_REGISTER_TYPE(ConstI32)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, ConstI32)

CEREAL_REGISTER_TYPE(ConstI64)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, ConstI64)

CEREAL_REGISTER_TYPE(ConstF32)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, ConstF32)

CEREAL_REGISTER_TYPE(ConstF64)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, ConstF64)

CEREAL_REGISTER_TYPE(ConstBool)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, ConstBool)

CEREAL_REGISTER_TYPE(ConstNull)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, ConstNull)

CEREAL_REGISTER_TYPE(BuildableTryCatch)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, BuildableTryCatch)

CEREAL_REGISTER_TYPE(BuildableFunction)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, BuildableFunction)

CEREAL_REGISTER_TYPE(BuildableType)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, BuildableType)

CEREAL_REGISTER_TYPE(BuildableString)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, BuildableString)

CEREAL_REGISTER_TYPE(RawOperation<>)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Buildable, RawOperation<>)



/*template <class...Ts>
struct Instruction {
public:
    Instruction()
    {
    }

    Instruction(const Instruction &other)
        : m_data(other.m_data)
    {
    }

    inline bool Empty() const
        { return !m_data.empty() && !m_data.back().empty(); }
    inline char GetOpcode() const
        { return m_data.back().back(); }

    std::vector<std::vector<char>> m_data;

protected:
    void Accept(NamesPair_t name)
    {
        std::vector<char> operand;

        char header[sizeof(name.first)];
        std::memcpy(&header[0], &name.first, sizeof(name.first));

        for (size_t j = 0; j < sizeof(name.first); j++) {
            operand.push_back(header[j]);
        }
        
        for (size_t j = 0; j < name.second.size(); j++) {
            operand.push_back(name.second[j]);
        }
        
        m_data.push_back(operand);
    }

    void Accept(std::vector<NamesPair_t> names)
    {
        std::vector<char> operand;

        for (size_t i = 0; i < names.size(); i++) {
            char header[sizeof(names[i].first)];
            std::memcpy(&header[0], &names[i].first, sizeof(names[i].first));

            for (size_t j = 0; j < sizeof(names[i].first); j++) {
                operand.push_back(header[j]);
            }
            for (size_t j = 0; j < names[i].second.size(); j++) {
                operand.push_back(names[i].second[j]);
            }
        }
        
        m_data.push_back(operand);
    }

    void Accept(const char *str)
    {
        // do not copy NUL byte
        size_t length = std::strlen(str);
        std::vector<char> operand;
        if (length) {
            operand.resize(length);
            std::memcpy(&operand[0], str, length);
        }
        m_data.push_back(operand);
    }

    template <typename T>
    void Accept(std::vector<T> ts)
    {
        std::vector<char> operand;
        operand.resize(sizeof(T) * ts.size());
        std::memcpy(&operand[0], &ts[0], sizeof(T) * ts.size());
        m_data.push_back(operand);
    }

    template <typename T>
    void Accept(T t)
    {
        std::vector<char> operand;
        operand.resize(sizeof(t));
        std::memcpy(&operand[0], &t, sizeof(t));
        m_data.push_back(operand);
    }

private:
    size_t pos = 0;
};

template <class T, class... Ts>
struct Instruction<T, Ts...> : Instruction<Ts...> {
    Instruction(T t, Ts...ts) : Instruction<Ts...>(ts...)
        { this->Accept(t); }
};*/

#endif
