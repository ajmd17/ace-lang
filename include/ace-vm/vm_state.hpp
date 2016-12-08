#ifndef VM_STATE_HPP
#define VM_STATE_HPP

#include <ace-vm/stack_memory.hpp>
#include <ace-vm/static_memory.hpp>
#include <ace-vm/heap_memory.hpp>
#include <ace-vm/exception.hpp>

#define GC_THRESHOLD_MUL 2
#define GC_THRESHOLD_MIN 20
#define GC_THRESHOLD_MAX 1000

struct Registers {
    StackValue m_reg[8];
    int m_flags = 0;

    inline StackValue &operator[](uint8_t index) { return m_reg[index]; }
    inline void ResetFlags() { m_flags = 0; }
};

struct ExceptionState {
    // incremented each time BEGIN_TRY is encountered,
    // decremented each time END_TRY is encountered
    int m_try_counter = 0;

    // set to true when an exception occurs,
    // set to false when handled in BEGIN_TRY
    bool m_exception_occured = false;
    inline void Reset() { m_try_counter = 0; m_exception_occured = false; }
};

struct ExecutionThread {
    Stack m_stack;
    ExceptionState m_exception_state;
    Registers m_regs;

    inline Stack &GetStack() { return m_stack; }
    inline ExceptionState &GetExceptionState() { return m_exception_state; }
    inline Registers &GetRegisters() { return m_regs; }
};

struct VMState {
    ExecutionThread m_exec_thread;
    Heap m_heap;
    StaticMemory m_static_memory;

    bool good = true;
    int m_max_heap_objects = GC_THRESHOLD_MIN;

    /** Reset the state of the VM, destroying all heap objects,
        stack objects and exception flags, etc.
     */
    void Reset();

    void ThrowException(const Exception &exception);
    HeapValue *HeapAlloc();

    inline ExecutionThread &GetExecutionThread() { return m_exec_thread; }
    inline Heap &GetHeap() { return m_heap; }
    inline StaticMemory &GetStaticMemory() { return m_static_memory; }
};

#endif