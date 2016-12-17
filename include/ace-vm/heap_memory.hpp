#ifndef HEAP_MEMORY_HPP
#define HEAP_MEMORY_HPP

#include <ace-vm/heap_value.hpp>

#include <ostream>

namespace ace {
namespace vm {

struct HeapNode {
    HeapValue value;
    HeapNode *before = nullptr;
    HeapNode *after = nullptr;
};

class Heap {
    friend std::ostream &operator<<(std::ostream &os, const Heap &heap);
public:
    Heap();
    Heap(const Heap &other) = delete;
    ~Heap();

    inline size_t Size() const { return m_num_objects; }

    /** Destroy everything on the heap */
    void Purge();
    /** Allocate a new value on the heap. */
    HeapValue *Alloc();
    /** Delete all nodes that are not marked */
    void Sweep();

private:
    HeapNode *m_head;
    size_t m_num_objects;
};

} // namespace vm
} // namespace ace

#endif
