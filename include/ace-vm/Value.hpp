#ifndef VALUE_HPP
#define VALUE_HPP

#include <ace-vm/HeapValue.hpp>

#include <common/typedefs.hpp>
#include <common/utf8.hpp>
#include <common/hasher.hpp>

#include <stdexcept>

namespace ace {
namespace vm {
    
struct Value {
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
        FUNCTION_CALL,
        TRY_CATCH_INFO
    } m_type;

    union ValueData {
        aint32 i32;
        aint64 i64;
        afloat32 f;
        afloat64 d;

        bool b;

        HeapValue *ptr;

        struct {
            uint32_t m_addr;
            uint8_t m_nargs;
            uint8_t m_flags;
        } func;

        NativeFunctionPtr_t native_func;
        
        struct {
            uint32_t addr;
            int32_t varargs_push;
        } call;

        uint32_t addr;

        struct {
            uint32_t catch_address;
        } try_catch_info;
    } m_value;

    //Value();
    Value() = default;
    explicit Value(const Value &other);

    inline Value::ValueType GetType()  const { return m_type; }
    inline Value::ValueData GetValue() const { return m_value; }

    inline std::uint32_t GetHash() const {
        utf::Utf8String tmp = ToString();
        return hash_fnv_1(tmp.GetData());
    }

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

    void Mark();

    const char *GetTypeString() const;
    utf::Utf8String ToString() const;
    void ToRepresentation(utf::Utf8String &out_str, bool add_type_name = true) const;
};

} // namespace vm
} // namespace ace

#endif
