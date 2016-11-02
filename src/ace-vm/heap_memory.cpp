#include <ace-vm/heap_memory.hpp>

std::ostream &operator<<(std::ostream &os, const Heap &heap)
{
    HeapNode *tmp_head = heap.m_head;
    while (tmp_head != nullptr) {
        os  << tmp_head->value.GetId() << "\t"
            << tmp_head->value.GetFlags() << "\t"
            << "\n";

        tmp_head = tmp_head->before;
    }
    return os;
}

Heap::Heap()
    : m_head(nullptr),
      m_num_objects(0)
{
}

Heap::~Heap()
{
    // clean up all allocated objects
    while (m_head != nullptr) {
        HeapNode *tmp = m_head;
        m_head = tmp->before;
        delete tmp;
    }
}

HeapValue *Heap::Alloc()
{
    HeapNode *node = new HeapNode;
    //node->value.GetFlags() |= GC_MARKED; // mark objects on first allocation

    if (m_head != nullptr) {
        m_head->after = node;
    }
    node->before = m_head;
    m_head = node;

    m_num_objects++;

    return &m_head->value;
}

void Heap::Sweep()
{
    HeapNode *last = m_head;
    while (last != nullptr) {
        if (!(last->value.GetFlags() & GC_MARKED)) {
            // unmarked object, so delete it

            HeapNode *after = last->after;
            HeapNode *before = last->before;

            if (before != nullptr) {
                before->after = after;
            }

            if (after != nullptr) {
                // removing an item from the middle, so
                // make the nodes to the other sides now
                // point to each other
                after->before = before;

                // delete the middle node
                delete last;
                last = before;
            } else {
                delete last;

                // since there are no nodes after this,
                // set the head to be this node here
                m_head = before;
                last = m_head;
            }

            // decrement number of currently allocated
            // objects
            m_num_objects--;

        } else {
            // the object is currently marked, so
            // we unmark it for the next time
            last->value.GetFlags() &= ~GC_MARKED;
            last = last->before;
        }
    }
}
