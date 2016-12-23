#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include <vector>
#include <ostream>
#include <cstring>
#include <iostream>

template <class...Ts>
struct Instruction {
public:
    /** Encodes data into the byte.
        opcode may be from 0-31,
        data may be from 0-3.
     */
    static unsigned char Encode(unsigned char opcode, unsigned char data);

public:
    Instruction() {}
    Instruction(const Instruction &other) : m_data(other.m_data) {}

    inline bool Empty() const { return !(m_data.empty()) && !(m_data.back().empty()); }
    inline char GetOpcode() const { return m_data.back().back(); }

    std::vector<std::vector<char>> m_data;

protected:
    void Accept(const char *str)
    {
        // do not copy NUL byte
        size_t length = std::strlen(str);
        std::vector<char> operand(length);
        std::memcpy(&operand[0], str, length);
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
    Instruction(T t, Ts...ts) : Instruction<Ts...>(ts...) { this->Accept(t); }
};

#endif
