#ifndef VM_HPP
#define VM_HPP

#include <ace-vm/bytecode_stream.hpp>
#include <ace-vm/vm_state.hpp>

#include <array>
#include <limits>
#include <cstdint>
#include <cstdio>

#define MAIN_THREAD m_state.m_threads[0]

#define THROW_COMPARISON_ERROR(left_type_str, right_type_str) \
    do { \
        char buffer[256]; \
        std::sprintf(buffer, "cannot compare '%s' with '%s'", \
            (left_type_str), (right_type_str)); \
        m_state.ThrowException(thread, Exception(buffer)); \
    } while (0)

#define IS_VALUE_INTEGER(value) \
    ((value).m_type == Value::I32 || \
    (value).m_type == Value::I64)

#define IS_VALUE_FLOATING_POINT(value) \
    ((value).m_type == Value::F32 || \
    (value).m_type == Value::F64)

#define IS_VALUE_STRING(value, out) \
    ((value).m_type == Value::HEAP_POINTER && \
    (out = (value).m_value.ptr->GetPointer<utf::Utf8String>()))

#define IS_VALUE_ARRAY(value, out) \
    ((value).m_type == Value::HEAP_POINTER && \
    (out = (value).m_value.ptr->GetPointer<Array>()))

#define MATCH_TYPES(left_type, right_type) \
    ((left_type) < (right_type)) ? (right_type) : (left_type)

#define COMPARE_FLOATING_POINT(lhs, rhs) \
    do { \
        if (IS_VALUE_FLOATING_POINT(rhs) || IS_VALUE_INTEGER(rhs)) { \
            double left = GetValueDouble(thread, lhs); \
            double right = GetValueDouble(thread, rhs); \
            if (left > right) { \
                thread->m_regs.m_flags = GREATER; \
            } else if (left == right) { \
                thread->m_regs.m_flags = EQUAL; \
            } else { \
                thread->m_regs.m_flags = NONE; \
            } \
        } else { \
            THROW_COMPARISON_ERROR(lhs, rhs); \
        } \
    } while (0) \

namespace ace {
namespace vm {

enum CompareFlags : int {
    NONE = 0x00,
    EQUAL = 0x01,
    GREATER = 0x02,
    // note that there is no LESS flag.
    // the compiler must make appropriate changes
    // to insure that the operands are switched to
    // use only the GREATER or EQUAL flags.
};

class VM {
public:
    VM();
    VM(const VM &other) = delete;
    ~VM();

    void PushNativeFunctionPtr(NativeFunctionPtr_t ptr);

    inline VMState &GetState() { return m_state; }
    inline const VMState &GetState() const { return m_state; }

    void Print(const Value &value);
    void Invoke(ExecutionThread *thread, BytecodeStream *bs, const Value &value, uint8_t num_args);
    void HandleInstruction(ExecutionThread *thread, BytecodeStream *bs, uint8_t code);
    void Execute();

private:
    VMState m_state;

    inline void CompareAsPointers(ExecutionThread *thread, Value *lhs, Value *rhs)
    {
        HeapValue *a = lhs->m_value.ptr;
        HeapValue *b = rhs->m_value.ptr;

        if (a == b) {
            // pointers equal, drop out early.
            thread->m_regs.m_flags = EQUAL;
        } else if (!a || !b) {
            // one of them is null, not equal
            thread->m_regs.m_flags = NONE;
        } else if (a->GetTypeId() == b->GetTypeId()) {
            // comparable types
            thread->m_regs.m_flags = (a->operator==(*b)) ? EQUAL : NONE;
        } else {
            THROW_COMPARISON_ERROR(lhs->GetTypeString(), rhs->GetTypeString());
        }
    }

    inline void CompareAsFunctions(ExecutionThread *thread, Value *lhs, Value *rhs)
    {
        if ((lhs->m_value.func.m_addr == rhs->m_value.func.m_addr) &&
            (rhs->m_value.func.m_nargs == lhs->m_value.func.m_nargs)) {
            thread->m_regs.m_flags = EQUAL;
        } else {
            thread->m_regs.m_flags = NONE;
        }
    }

    inline void CompareAsNativeFunctions(ExecutionThread *thread, Value *lhs, Value *rhs)
    {
        thread->m_regs.m_flags = (lhs->m_value.native_func == rhs->m_value.native_func) ? EQUAL : NONE;
    }

    /** Returns the value as a 64-bit integer without checking if it is not of that type. */
    inline int64_t GetIntFast(const Value &value)
    {
        if (value.m_type == Value::I64) {
            return value.m_value.i64;
        }
        return (int64_t)value.m_value.i32;
    }

    /** Returns the value as a 64-bit integer, throwing an error if the type is invalid. */
    inline int64_t GetValueInt64(ExecutionThread *thread, const Value &value)
    {
        switch (value.m_type) {
        case Value::I32:
            return (int64_t)value.m_value.i32;
        case Value::I64:
            return value.m_value.i64;
        case Value::F32:
            return (int64_t)value.m_value.f;
        case Value::F64:
            return (int64_t)value.m_value.d;
        default:
        {
            char buffer[256];
            std::sprintf(buffer, "no conversion from '%s' to 'Int64'", value.GetTypeString());
            m_state.ThrowException(thread, Exception(buffer));

            return 0;
        }
        }
    }

    inline double GetValueDouble(ExecutionThread *thread, const Value &value)
    {
        switch (value.m_type) {
        case Value::I32:
            return (double)value.m_value.i32;
        case Value::I64:
            return (double)value.m_value.i64;
        case Value::F32:
            return (double)value.m_value.f;
        case Value::F64:
            return value.m_value.d;
        default:
        {
            char buffer[256];
            std::sprintf(buffer, "no conversion from '%s' to 'Float64'",
                value.GetTypeString());
            m_state.ThrowException(thread, Exception(buffer));

            return std::numeric_limits<double>::quiet_NaN();
        }
        }
    }
};

} // namespace vm
} // namespace ace

#endif
