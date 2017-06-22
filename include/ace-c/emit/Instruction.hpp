#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include <ace-c/emit/NamesPair.hpp>
#include <common/instructions.hpp>

#include <vector>
#include <ostream>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <streambuf>

using Opcode = uint8_t;
using LabelId = size_t;
using LabelPosition = uint32_t;
using byte = uint8_t;
using Buffer = std::basic_streambuf<byte>;

struct Label {
    LabelPosition position;
};

struct BuildParams {
    size_t block_offset;
    size_t local_offset;
    std::vector<Label> labels;

    BuildParams() : block_offset(0), local_offset(0) {}
};

struct Buildable {
    virtual size_t GetSize() const = 0;
    virtual void Build(Buffer &buf, BuildParams &build_params) const = 0;
};

struct Instruction : public Buildable {
    Opcode opcode;

    virtual size_t GetSize() const override = 0;
    virtual void Build(Buffer &buf, BuildParams &build_params) const override = 0;
};

struct Jump final : public Instruction {
    LabelId label_id;

    virtual size_t GetSize() const override
    {
        return sizeof(opcode) + sizeof(LabelPosition);
    }

    virtual void Build(Buffer &buf, BuildParams &build_params) const override
    {
        buf.sputc(opcode);

        LabelPosition pos = build_params.block_offset
            + build_params.labels[label_id].position;

        buf.sputn((byte*)&pos, sizeof(pos));
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
        buf.sputc(BEGIN_TRY);

        LabelPosition pos = build_params.block_offset
            + build_params.labels[catch_label_id].position;

        buf.sputn((byte*)&pos, sizeof(pos));
    }
};

struct BuildableFunction final : public Buildable {
    uint8_t reg;
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
        buf.sputc(LOAD_FUNC);
        buf.sputc(reg);
        buf.sputn((byte*)&pos, sizeof(pos));
        buf.sputc(nargs);
        buf.sputc(flags);
    }
};

struct BuildableType final : public Buildable {
    uint8_t reg;
    std::string name;
    std::vector<std::string> members;

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
        buf.sputc(LOAD_TYPE);
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
};

struct BuildableString final : public Buildable {
    uint8_t reg;
    std::string value;

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
        buf.sputc(LOAD_STRING);
        buf.sputc(reg);
        buf.sputn((byte*)&len, sizeof(len));
        buf.sputn((byte*)&value[0], value.length());
    }
};

struct BinOp final : public Instruction {
    uint8_t reg_lhs;
    uint8_t reg_rhs;
    uint8_t reg_dst;

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
};

template <class T, class... Ts>
struct RawOperation<T, Ts...> : RawOperation<Ts...> {
    RawOperation(T t, Ts...ts) : RawOperation<Ts...>(ts...)
        { this->Accept(t); }
};

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
