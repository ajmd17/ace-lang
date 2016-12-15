#include <ace-vm/vm_state.hpp>

#include <common/utf8.hpp>

#include <algorithm>
#include <thread>
#include <iostream>
#include <mutex>

static std::mutex mtx;

VMState::VMState()
{
    for (int i = 0; i < VM_MAX_THREADS; i++) {
        m_threads[i] = nullptr;
    }
}

VMState::~VMState()
{
    Reset();
}

void VMState::Reset()
{
    // purge the heap
    m_heap.Purge();
    // reset heap threshold
    m_max_heap_objects = GC_THRESHOLD_MIN;
    // purge static memory
    m_static_memory.Purge();

    for (int i = 0; i < VM_MAX_THREADS; i++) {
        DestroyThread(i);
    }

    // we're good to go
    good = true;
}

void VMState::ThrowException(ExecutionThread *thread, const Exception &exception)
{
    thread->m_exception_state.m_exception_occured = true;

    if (thread->m_exception_state.m_try_counter <= 0) {
        // exception cannot be handled, no try block found
        if (thread->m_id == 0) {
            utf::printf(UTF8_CSTR("unhandled exception in main thread: %" PRIutf8s "\n"),
                UTF8_TOWIDE(exception.ToString().GetData()));
        } else {
            utf::printf(UTF8_CSTR("unhandled exception in thread #%d: %" PRIutf8s "\n"),
                thread->m_id + 1, UTF8_TOWIDE(exception.ToString().GetData()));
        }

        good = false;
    }
}

HeapValue *VMState::HeapAlloc(ExecutionThread *thread)
{
    ASSERT(thread != nullptr);

    size_t heap_size = m_heap.Size();
    
    if (heap_size >= GC_THRESHOLD_MAX) {
        // heap overflow.
        char buffer[256];
        std::sprintf(buffer, "heap overflow, heap size is %zu", heap_size);
        ThrowException(thread, Exception(buffer));
        return nullptr;
    } else if (heap_size >= m_max_heap_objects) {
        // run the gc
        GC();

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

void VMState::GC()
{
    std::lock_guard<std::mutex> lock(mtx);

    // mark stack objects on each thread
    for (int i = 0; i < VM_MAX_THREADS; i++) {
        if (m_threads[i] != nullptr) {
            m_threads[i]->m_stack.MarkAll();
            for (int j = 0; j < VM_NUM_REGISTERS; j++) {
                m_threads[i]->GetRegisters()[j].Mark();
            }
        }
    }

    m_heap.Sweep();

    utf::cout << "gc()\n";
}

ExecutionThread *VMState::CreateThread()
{
    ASSERT(CanCreateThread());

    // find free slot
    for (int i = 0; i < VM_MAX_THREADS; i++) {
        if (m_threads[i] == nullptr) {
            ExecutionThread *thread = new ExecutionThread();
            thread->m_id = i;

            m_threads[i] = thread;
            m_num_threads++;

            return thread;
        }
    }

    return nullptr;
}

void VMState::DestroyThread(int id)
{
    ASSERT(id < VM_MAX_THREADS);

    ExecutionThread *thread = m_threads[id];

    if (thread != nullptr) {
        // purge the stack
        thread->m_stack.Purge();
        // reset exception state
        thread->m_exception_state.Reset();
        // reset register flags
        thread->m_regs.ResetFlags();

        // delete it
        delete m_threads[id];
        m_threads[id] = nullptr;

        m_num_threads--;
    }
}
