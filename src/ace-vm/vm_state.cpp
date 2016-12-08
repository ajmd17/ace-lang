#include <ace-vm/vm_state.hpp>

#include <common/utf8.hpp>

#include <algorithm>

void VMState::Reset()
{
    // purge the heap
    m_heap.Purge();
    // reset heap threshold
    m_max_heap_objects = GC_THRESHOLD_MIN;
    // purge static memory
    m_static_memory.Purge();
    // purge the stack
    m_exec_thread.m_stack.Purge();
    // reset exception state
    m_exec_thread.m_exception_state.Reset();
    // reset register flags
    m_exec_thread.m_regs.ResetFlags();
    // we're good to go
    good = true;
}

void VMState::ThrowException(const Exception &exception)
{
    m_exec_thread.m_exception_state.m_exception_occured = true;
    if (m_exec_thread.m_exception_state.m_try_counter <= 0) {
        // exception cannot be handled
        // unhandled exception error
        utf::printf(UTF8_CSTR("unhandled exception: %" PRIutf8s "\n"),
            UTF8_TOWIDE(exception.ToString().GetData()));

        good = false;
    }
}

HeapValue *VMState::HeapAlloc()
{
    int heap_size = m_heap.Size();
    if (heap_size >= GC_THRESHOLD_MAX) {
        // heap overflow.
        char buffer[256];
        std::sprintf(buffer, "heap overflow, heap size is %d", heap_size);
        ThrowException(Exception(buffer));
        return nullptr;
    } else if (heap_size >= m_max_heap_objects) {
        // run the gc
        m_exec_thread.m_stack.MarkAll();
        m_heap.Sweep();

        utf::cout << "gc()\n";

        // check if size is still over the maximum,
        // and resize the maximum if necessary.
        if (m_heap.Size() >= m_max_heap_objects) {
            // resize max number of objects
            m_max_heap_objects = std::min(
                m_max_heap_objects * GC_THRESHOLD_MUL, GC_THRESHOLD_MAX);
        }
    }

    return m_heap.Alloc();
}