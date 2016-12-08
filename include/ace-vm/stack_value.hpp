#ifndef STACK_VALUE_HPP
#define STACK_VALUE_HPP

#include <ace-vm/heap_value.hpp>

#include <common/utf8.hpp>

#include <stdexcept>
#include <cstdint>

// forward declarations
struct StackValue;
struct VMState;

typedef void(*NativeFunctionPtr_t)(VMState*, StackValue**, int);

struct Function {
    uint32_t m_addr;
    uint8_t m_nargs;
};

struct StackValue {
    enum ValueType {
        INT32,
        INT64,
        FLOAT,
        DOUBLE,
        BOOLEAN,
        HEAP_POINTER,
        FUNCTION,
        NATIVE_FUNCTION,
        ADDRESS
    } m_type;

    union ValueData {
        int32_t i32;
        int64_t i64;
        float f;
        double d;
        bool b;
        HeapValue *ptr;
        Function func;
        NativeFunctionPtr_t native_func;
        uint32_t addr;
    } m_value;

    StackValue();
    explicit StackValue(const StackValue &other);

    inline StackValue::ValueType GetType() const { return m_type; }
    inline StackValue::ValueData GetValue() const { return m_value; }

    void Mark();
    utf::Utf8String ToString();

    inline const char *GetTypeString() const
    {
        switch (m_type) {
        case INT32:
            return "Int32";
        case INT64:
            return "Int64";
        case FLOAT:
            return "Float32";
        case DOUBLE:
            return "Float64";
        case BOOLEAN:
            return "Bool";
        case HEAP_POINTER:
            return "Object";
        case FUNCTION:
            return "Function";
        case NATIVE_FUNCTION:
            return "NativeFunction";
        default:
            return "Undefined";
        }
    }
};

#endif
