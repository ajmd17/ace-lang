#ifndef VALUE_HPP
#define VALUE_HPP

#include <ace-vm/HeapValue.hpp>

#include <common/typedefs.hpp>
#include <common/utf8.hpp>

#include <stdexcept>

namespace ace {
namespace vm {

struct Value {
    union ValueData {
        aint32 i32;
        aint64 i64;
        afloat32 f;
        afloat64 d;

        bool b;

        HeapValue *ptr;

        struct {
            uint32_t m_addr;
            uint32_t m_nargs;
        } func;

        NativeFunctionPtr_t native_func;
        uint32_t addr;

        struct {
            uint32_t catch_address;
        } try_catch_info;
    } m_value;

    enum ValueType {
        /* These first four types are listed in order of precedence */
        I32,
        I64,
        F32,
        F64,

        BOOLEAN,

        HEAP_POINTER,
        FUNCTION,
        NATIVE_FUNCTION,
        ADDRESS,
        TRY_CATCH_INFO
    } m_type;

    Value();
    explicit Value(const Value &other);

    inline Value::ValueType GetType()  const { return m_type; }
    inline Value::ValueData GetValue() const { return m_value; }

    inline bool GetInteger(aint64 *out) const
    {
        switch (m_type) {
            case I32: *out = m_value.i32; return true;
            case I64: *out = m_value.i64; return true;
            default:                      return false;
        }
    }

    inline bool GetFloatingPoint(afloat64 *out) const
    {
        switch (m_type) {
            case F32: *out = m_value.f; return true;
            case F64: *out = m_value.d; return true;
            default:                    return false;
        }
    }

    inline bool GetNumber(afloat64 *out) const
    {
        switch (m_type) {
            case I32: *out = m_value.i32; return true;
            case I64: *out = m_value.i64; return true;
            case F32: *out = m_value.f;   return true;
            case F64: *out = m_value.d;   return true;
            default:                      return false;
        }
    }

    inline const char *GetTypeString() const
    {
        switch (m_type) {
            case I32: return "Int32";
            case I64: return "Int64";
            case F32: return "Float32";
            case F64: return "Float64";
            case BOOLEAN: return "Boolean";
            case HEAP_POINTER: return "Object";
            case FUNCTION: return "Function";
            case NATIVE_FUNCTION: return "NativeFunction";
            default: return "??";
        }
    }

    void Mark();
    utf::Utf8String ToString();
};

} // namespace vm
} // namespace ace

#endif
