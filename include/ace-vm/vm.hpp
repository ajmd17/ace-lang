#ifndef VM_HPP
#define VM_HPP

#include <ace-vm/bytecode_stream.hpp>
#include <ace-vm/stack_memory.hpp>
#include <ace-vm/static_memory.hpp>
#include <ace-vm/heap_memory.hpp>
#include <ace-vm/exception.hpp>

#include <array>
#include <limits>
#include <cstdint>
#include <cstdio>

#define GC_THRESHOLD_MUL 2
#define GC_THRESHOLD_MIN 20
#define GC_THRESHOLD_MAX 1000

#define THROW_COMPARISON_ERROR(lhs, rhs) \
    do { \
        char buffer[256]; \
        std::sprintf(buffer, "cannot compare '%s' with '%s'", \
            lhs.GetTypeString(), rhs.GetTypeString()); \
        ThrowException(Exception(buffer)); \
    } while (0)

#define IS_VALUE_INTEGER(stack_value) \
    ((stack_value).m_type == StackValue::INT32 || \
    (stack_value).m_type == StackValue::INT64)

#define IS_VALUE_FLOATING_POINT(stack_value) \
    ((stack_value).m_type == StackValue::FLOAT || \
    (stack_value).m_type == StackValue::DOUBLE)

#define MATCH_TYPES(lhs, rhs) \
    ((lhs).m_type < (rhs).m_type) ? (rhs).m_type : (lhs).m_type

#define COMPARE_FLOATING_POINT(lhs, rhs) \
    do { \
        if (IS_VALUE_FLOATING_POINT(rhs) || IS_VALUE_INTEGER(rhs)) { \
            double left = GetValueDouble(lhs); \
            double right = GetValueDouble(rhs); \
            if (left > right) { \
                m_exec_thread.m_regs.m_flags = GREATER; \
            } else if (left == right) { \
                m_exec_thread.m_regs.m_flags = EQUAL; \
            } else { \
                m_exec_thread.m_regs.m_flags = NONE; \
            } \
        } else { \
            THROW_COMPARISON_ERROR(lhs, rhs); \
        } \
    } while (0) \

#define COMPARE_REFERENCES(lhs, rhs) \
    do { \
        if (rhs.m_type == StackValue::HEAP_POINTER) { \
            if (lhs.m_value.ptr > rhs.m_value.ptr) { \
                m_exec_thread.m_regs.m_flags = GREATER; \
            } else if (lhs.m_value.ptr == rhs.m_value.ptr) { \
                m_exec_thread.m_regs.m_flags = EQUAL; \
            } else { \
                m_exec_thread.m_regs.m_flags = NONE; \
            } \
        } else { \
            THROW_COMPARISON_ERROR(lhs, rhs); \
        } \
    } while(0)

#define COMPARE_FUNCTIONS(lhs, rhs) \
    do { \
        if (rhs.m_type == StackValue::FUNCTION) { \
            if (lhs.m_value.func.m_addr > rhs.m_value.func.m_addr) { \
                m_exec_thread.m_regs.m_flags = GREATER; \
            } else if (lhs.m_value.func.m_addr == rhs.m_value.func.m_addr && \
                rhs.m_value.func.m_nargs == lhs.m_value.func.m_nargs) { \
                m_exec_thread.m_regs.m_flags = EQUAL; \
            } else { \
                m_exec_thread.m_regs.m_flags = NONE; \
            } \
        } else { \
            THROW_COMPARISON_ERROR(lhs, rhs); \
        } \
    } while(0)

enum CompareFlags : int {
    NONE = 0x00,
    EQUAL = 0x01,
    GREATER = 0x02,
    // note that there is no LESS flag.
    // the compiler must make appropriate changes
    // to insure that the operands are switched to
    // use only the GREATER or EQUAL flags.
};

struct Registers {
    StackValue m_reg[8];
    int m_flags = 0;

    inline StackValue &operator[](uint8_t index) { return m_reg[index]; }
};

struct ExceptionState {
    // incremented each time BEGIN_TRY is encountered,
    // decremented each time END_TRY is encountered
    int m_try_counter = 0;

    // set to true when an exception occurs,
    // set to false when handled in BEGIN_TRY
    bool m_exception_occured = false;
};

struct ExecutionThread {
    Stack m_stack;
    ExceptionState m_exception_state;
    Registers m_regs;
};

class VM {
public:
    VM(BytecodeStream *bs);
    VM(const VM &other) = delete;
    ~VM();

    inline Heap &GetHeap() { return m_heap; }
    inline ExecutionThread &GetExecutionThread() { return m_exec_thread; }

    HeapValue *HeapAlloc();
    void MarkObject(StackValue &object);
    void MarkObjects(ExecutionThread *thread);
    void Echo(StackValue &value);
    void InvokeFunction(StackValue &value, uint8_t num_args);
    void HandleInstruction(uint8_t code);
    void Execute();

private:
    StaticMemory m_static_memory;
    Heap m_heap;
    ExecutionThread m_exec_thread;
    int m_max_heap_objects;

    BytecodeStream *m_bs;

    void ThrowException(const Exception &exception);

    inline bool HasNextInstruction() const { return m_bs->Position() < m_bs->Size(); }

    inline int64_t GetValueInt64(const StackValue &stack_value)
    {
        switch (stack_value.m_type) {
        case StackValue::INT32:
            return (int64_t)stack_value.m_value.i32;
        case StackValue::INT64:
            return stack_value.m_value.i64;
        case StackValue::FLOAT:
            return (int64_t)stack_value.m_value.f;
        case StackValue::DOUBLE:
            return (int64_t)stack_value.m_value.d;
        case StackValue::BOOLEAN:
            return (int64_t)stack_value.m_value.b;
        default:
        {
            char buffer[256];
            std::sprintf(buffer, "no conversion from '%s' to 'Int64'",
                stack_value.GetTypeString());
            ThrowException(Exception(buffer));

            return 0;
        }
        }
    }

    inline double GetValueDouble(const StackValue &stack_value)
    {
        switch (stack_value.m_type) {
        case StackValue::INT32:
            return (double)stack_value.m_value.i32;
        case StackValue::INT64:
            return (double)stack_value.m_value.i64;
        case StackValue::FLOAT:
            return (double)stack_value.m_value.f;
        case StackValue::DOUBLE:
            return stack_value.m_value.d;
        case StackValue::BOOLEAN:
            return (double)stack_value.m_value.b;
        default:
        {
            char buffer[256];
            std::sprintf(buffer, "no conversion from '%s' to 'Double'",
                stack_value.GetTypeString());
            ThrowException(Exception(buffer));

            return std::numeric_limits<double>::quiet_NaN();
        }
        }
    }
};

#endif
