#ifndef VM_STATE_HPP
#define VM_STATE_HPP

#include <ace-vm/stack_memory.hpp>
#include <ace-vm/static_memory.hpp>
#include <ace-vm/heap_memory.hpp>

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

struct VMState {
    ExecutionThread m_exec_thread;
    Heap m_heap;
    StaticMemory m_static_memory;

    bool good = true;
};

#endif