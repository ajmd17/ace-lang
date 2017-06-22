#ifndef CPP_GENERATOR_HPP
#define CPP_GENERATOR_HPP

#include <ace-c/emit/InstructionStream.hpp>
#include <ace-vm/BytecodeStream.hpp>

#include <sstream>

class CppGenerator {
    friend std::ostream &operator<<(std::ostream &os, CppGenerator &cpp_gen);
public:
    CppGenerator();
    CppGenerator(const CppGenerator &other) = delete;

    CppGenerator &operator<<(ace::vm::BytecodeStream &bs);
    //CppGenerator &operator<<(InstructionStream &is);

private:
    std::stringstream m_cpp_ss;

    void OutputArgument(uint8_t u8) {
        m_cpp_ss << (int)u8;
    }

    void OutputArgument(char *str) {
        m_cpp_ss << "\"" << str << "\"";
    }

    void OutputArgument(const char *str) {
        m_cpp_ss << "\"" << str << "\"";
    }

    void OutputArgument(const std::string &str) {
        m_cpp_ss << "\"" << str << "\"";
    }

    template<class T>
    void OutputArgument(T &&t) {
        m_cpp_ss << t;
    }

    template<class T, class... Ts>
    void OutputArgument(T &&t, Ts &&...ts) {
        OutputArgument(t);
        m_cpp_ss << ", ";
        OutputArgument(std::forward<Ts>(ts)...);
    }

    template<class... Args>
    void AddInstructionCall(const std::string &name, Args &&...args)
    {
        m_cpp_ss << "  vm." << name << "(";
        OutputArgument(std::forward<Args>(args)...);
        m_cpp_ss << ");\n";
    }

    inline void AppendCppCode(const std::string &code_str, bool indent = true)
    {
        if (indent) {
            m_cpp_ss << "  ";
        }
        m_cpp_ss << code_str;
    }
};

#endif