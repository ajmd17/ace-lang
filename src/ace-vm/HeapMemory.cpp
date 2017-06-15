#include <ace-vm/HeapMemory.hpp>

#include <ace-vm/Object.hpp>
#include <ace-vm/Array.hpp>
#include <ace-vm/Value.hpp>
#include <ace-vm/TypeInfo.hpp>

#include <common/utf8.hpp>

#include <iostream>
#include <iomanip>
#include <bitset>

namespace ace {
namespace vm {

std::ostream &operator<<(std::ostream &os, const Heap &heap)
{
    // print table header
    os << std::left;
    os << std::setw(16) << "Address" << "| ";
    os << std::setw(8) << "Flags" << "| ";
    os << std::setw(10) << "Type" << "| ";
    os << std::setw(16) << "Value";
    os << std::endl;

    HeapNode *tmp_head = heap.m_head;
    while (tmp_head) {
        os << std::setw(16) << (void*)tmp_head->value.GetId() << "| ";
        os << std::setw(8) << std::bitset<sizeof(tmp_head->value.GetFlags())>(tmp_head->value.GetFlags()) << "| ";
        os << std::setw(10);

        {
            utf::Utf8String *str_ptr = nullptr;
            Array *array_ptr = nullptr;
            Object *obj_ptr = nullptr;
            TypeInfo *type_info_ptr = nullptr;
            
            if (!tmp_head->value.GetId()) {
                os << "NullType" << "| ";

                os << std::setw(16);
                os << "null";
            } else if ((str_ptr = tmp_head->value.GetPointer<utf::Utf8String>()) != nullptr) {
                os << "String" << "| ";

                os << "\"" << *str_ptr << "\"" << std::setw(16);
            } else if ((array_ptr = tmp_head->value.GetPointer<Array>()) != nullptr) {
                os << "Array" << "| ";

                os << std::setw(16);
                utf::Utf8String tmp_str(256);
                array_ptr->GetRepresentation(tmp_str, false);
                os << tmp_str;
            } else if ((obj_ptr = tmp_head->value.GetPointer<Object>()) != nullptr) {
                ASSERT(obj_ptr->GetTypePtr() != nullptr);
                os << obj_ptr->GetTypePtr()->GetName() << "| ";

                os << std::setw(16);
                utf::Utf8String tmp_str(256);
                obj_ptr->GetRepresentation(tmp_str, false);
                os << tmp_str;
            } else if ((type_info_ptr = tmp_head->value.GetPointer<TypeInfo>()) != nullptr) {
                os << "TypeInfo" << "| ";

                os << std::setw(16);
                os << " ";
            } else {
                os << "Pointer" << "| ";

                os << std::setw(16);
                os << " ";
            }
        }

        os << std::endl;
        /*os  << tmp_head->value.GetId() << '\t'
            << tmp_head->value.GetFlags() << '\t'
            << '\n';*/

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
    // purge the heap on destructor
    Purge();
}

void Heap::Purge()
{
    // clean up all allocated objects
    while (m_head) {
        HeapNode *tmp = m_head;
        m_head = tmp->before;
        delete tmp;

        m_num_objects--;
    }
}

HeapValue *Heap::Alloc()
{
    HeapNode *node = new HeapNode;

    node->after = nullptr;
    
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
    while (last) {
        if (!(last->value.GetFlags() & GC_MARKED)) {
            // unmarked object, so delete it

            HeapNode *after = last->after;
            HeapNode *before = last->before;

            if (before) {
                before->after = after;
            }

            if (after) {
                // removing an item from the middle, so
                // make the nodes to the other sides now
                // point to each other
                after->before = before;
            } else {
                // since there are no nodes after this,
                // set the head to be this node here
                m_head = before;
            }

            // delete the middle node
            delete last;
            last = before;

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

} // namespace vm
} // namespace ace
