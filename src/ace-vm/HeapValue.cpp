#include <ace-vm/HeapValue.hpp>
#include <ace-vm/Object.hpp>
#include <ace-vm/Array.hpp>
#include <ace-vm/Slice.hpp>
#include <ace-vm/ImmutableString.hpp>

#include <common/my_assert.hpp>

namespace ace {
namespace vm {

HeapValue::HeapValue()
    : m_holder(nullptr),
      m_ptr(nullptr),
      m_flags(0)
{
}

HeapValue::~HeapValue()
{
    if (m_holder != nullptr) {
        delete m_holder;
    }
}

void HeapValue::Mark()
{
    ASSERT_MSG(!(m_flags & GC_MARKED), "doubly marked"); // there have been issues when marking right after allocating

    if (Object *object = GetPointer<Object>()) {
        HeapValue *proto = nullptr;

        size_t iter = 0;

        do {
            const size_t size = object->GetSize();

            for (size_t i = 0; i < size; i++) {
                object->GetMember(i).value.Mark();
            }

            if ((proto = object->GetPrototype()) != nullptr) {
                proto->GetFlags() |= GC_MARKED;

                // get object value of prototype
                if ((object = proto->GetPointer<Object>()) == nullptr) {
                    // if prototype is not an object (has been modified to another value), we're done.
                    return;
                }
            }

            iter++;
        } while (proto != nullptr);
    } else if (Array *array = GetPointer<Array>()) {
        const size_t size = array->GetSize();

        for (int i = 0; i < size; i++) {
            array->AtIndex(i).Mark();
        }
    } else if (Slice *slice = GetPointer<Slice>()) {
        const size_t size = slice->GetSize();

        for (int i = 0; i < size; i++) {
            slice->AtIndex(i).Mark();
        }
    }
}

} // namespace vm
} // namespace ace