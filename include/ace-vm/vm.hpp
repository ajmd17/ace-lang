#ifndef VM_HPP
#define VM_HPP

#include <ace-vm/bytecode_stream.hpp>
#include <ace-vm/vm_state.hpp>

#include <array>
#include <limits>
#include <cstdint>
#include <cstdio>

#define MAIN_THREAD m_state.m_threads[0]

#define THROW_COMPARISON_ERROR(lhs, rhs) \
    do { \
        char buffer[256]; \
        std::sprintf(buffer, "cannot compare '%s' with '%s'", \
            lhs.GetTypeString(), rhs.GetTypeString()); \
        m_state.ThrowException(thread, Exception(buffer)); \
    } while (0)

#define IS_VALUE_INTEGER(stack_value) \
    ((stack_value).m_type == StackValue::I32 || \
    (stack_value).m_type == StackValue::I64)

#define IS_VALUE_FLOATING_POINT(stack_value) \
    ((stack_value).m_type == StackValue::F32 || \
    (stack_value).m_type == StackValue::F64)

#define IS_VALUE_STRING(stack_value, out) \
    ((stack_value).m_type == StackValue::HEAP_POINTER && \
    (out = (stack_value).m_value.ptr->GetPointer<utf::Utf8String>()))

#define IS_VALUE_ARRAY(stack_value, out) \
    ((stack_value).m_type == StackValue::HEAP_POINTER && \
    (out = (stack_value).m_value.ptr->GetPointer<Array>()))

#define MATCH_TYPES(lhs, rhs) \
    ((lhs).m_type < (rhs).m_type) ? (rhs).m_type : (lhs).m_type

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

    void Print(const StackValue &value);
    void Invoke(ExecutionThread *thread, BytecodeStream *bs, const StackValue &value, uint8_t num_args);
    void HandleInstruction(ExecutionThread *thread, BytecodeStream *bs, uint8_t code);
    void Execute();

private:
    VMState m_state;

    inline void CompareAsFloats(ExecutionThread *thread, const StackValue &lhs, const StackValue &rhs)
    {
        if (IS_VALUE_FLOATING_POINT(rhs) || IS_VALUE_INTEGER(rhs)) {
            double left = GetValueDouble(thread, lhs);
            double right = GetValueDouble(thread, rhs);
            if (left > right) {
                thread->m_regs.m_flags = GREATER;
            } else if (left == right) {
                thread->m_regs.m_flags = EQUAL;
            } else {
                thread->m_regs.m_flags = NONE;
            }
        } else {
            THROW_COMPARISON_ERROR(lhs, rhs);
        }
    }

    inline void CompareAsPointers(ExecutionThread *thread, const StackValue &lhs, const StackValue &rhs)
    {
        if (lhs.m_value.ptr == rhs.m_value.ptr) {
            // pointers equal, drop out early.
            thread->m_regs.m_flags = EQUAL;
        } else if (lhs.m_value.ptr == nullptr || rhs.m_value.ptr == nullptr) {
            // one of them is null, not equal
            thread->m_regs.m_flags = NONE;
        } else if (lhs.m_value.ptr->GetTypeId() == rhs.m_value.ptr->GetTypeId()) {
            // comparable types
            if (lhs.m_value.ptr->operator==(*rhs.m_value.ptr)) {
                thread->m_regs.m_flags = EQUAL;
            } else {
                thread->m_regs.m_flags = NONE;
            }
        } else {
            THROW_COMPARISON_ERROR(lhs, rhs);
        }
    }

    inline void CompareAsFunctions(ExecutionThread *thread, const StackValue &lhs, const StackValue &rhs)
    {
        if ((lhs.m_value.func.m_addr == rhs.m_value.func.m_addr) &&
            (rhs.m_value.func.m_nargs == lhs.m_value.func.m_nargs)) {
            thread->m_regs.m_flags = EQUAL;
        } else {
            thread->m_regs.m_flags = NONE;
        }
    }

    inline void CompareAsNativeFunctions(ExecutionThread *thread, const StackValue &lhs, const StackValue &rhs)
    {
        if (lhs.m_value.native_func == rhs.m_value.native_func) {
            thread->m_regs.m_flags = EQUAL;
        } else {
            thread->m_regs.m_flags = NONE;
        }
    }

    /** Returns the value as a 64-bit integer without checking if it is not of that type. */
    inline int64_t GetIntFast(const StackValue &stack_value) 
    {
        if (stack_value.m_type == StackValue::I64) {
            return stack_value.m_value.i64;
        }
        return (int64_t)stack_value.m_value.i32;
    }

    /** Returns the value as a 64-bit integer, throwing an error if the type is invalid. */
    inline int64_t GetValueInt64(ExecutionThread *thread, const StackValue &stack_value)
    {
        switch (stack_value.m_type) {
        case StackValue::I32:
            return (int64_t)stack_value.m_value.i32;
        case StackValue::I64:
            return stack_value.m_value.i64;
        case StackValue::F32:
            return (int64_t)stack_value.m_value.f;
        case StackValue::F64:
            return (int64_t)stack_value.m_value.d;
        default:
        {
            char buffer[256];
            std::sprintf(buffer, "no conversion from '%s' to 'Int64'",
                stack_value.GetTypeString());
            m_state.ThrowException(thread, Exception(buffer));

            return 0;
        }
        }
    }

    inline double GetValueDouble(ExecutionThread *thread, const StackValue &stack_value)
    {
        switch (stack_value.m_type) {
        case StackValue::I32:
            return (double)stack_value.m_value.i32;
        case StackValue::I64:
            return (double)stack_value.m_value.i64;
        case StackValue::F32:
            return (double)stack_value.m_value.f;
        case StackValue::F64:
            return stack_value.m_value.d;
        default:
        {
            char buffer[256];
            std::sprintf(buffer, "no conversion from '%s' to 'Float64'",
                stack_value.GetTypeString());
            m_state.ThrowException(thread, Exception(buffer));

            return std::numeric_limits<double>::quiet_NaN();
        }
        }
    }
};

#endif
