#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <vector>
#include <ostream>
#include <cstring>

template <class...Ts>
struct Instruction {
public:
    Instruction()
    {
    }

    Instruction(const Instruction &other)
        : m_data(other.m_data)
    {
    }

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
    {
        this->Accept(t);
    }
};

#endif