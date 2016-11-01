#ifndef STACK_VALUE_HPP
#define STACK_VALUE_HPP

#include <ace-vm/heap_value.hpp>

#include <stdexcept>
#include <cstdint>

struct Function {
    uint32_t m_addr;
    uint8_t m_nargs;
};

struct TypeInfo {
    uint8_t m_size;
};

struct StackValue {
    enum {
        INT32,
        INT64,
        FLOAT,
        DOUBLE,
        BOOLEAN,
        HEAP_POINTER,
        FUNCTION,
        ADDRESS,
        TYPE_INFO,
    } m_type;

    union {
        int32_t i32;
        int64_t i64;
        float f;
        double d;
        bool b;
        HeapValue *ptr;
        Function func;
        uint32_t addr;
        TypeInfo type_info;
    } m_value;

    StackValue();
    explicit StackValue(const StackValue &other);

    inline const char *GetTypeString() const
    {
        switch (m_type) {
        case INT32:
            return "int32";
        case INT64:
            return "int64";
        case FLOAT:
            return "float";
        case DOUBLE:
            return "double";
        case BOOLEAN:
            return "boolean";
        case HEAP_POINTER:
            return "reference";
        case FUNCTION:
            return "function";
        default:
            return "undefined";
        }
    }
};

#endif
