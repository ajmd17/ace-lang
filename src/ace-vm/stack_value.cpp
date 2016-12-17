#include <ace-vm/stack_value.hpp>
#include <ace-vm/object.hpp>
#include <ace-vm/array.hpp>

#include <stdio.h>
#include <cinttypes>

namespace ace {
namespace vm {

Value::Value()
    : m_type(HEAP_POINTER)
{
    // initialize to null reference
    m_value.ptr = nullptr;
}

Value::Value(const Value &other)
    : m_type(other.m_type),
      m_value(other.m_value)
{
}

void Value::Mark()
{
    HeapValue *ptr = m_value.ptr;
    
    if (m_type == Value::HEAP_POINTER && ptr != nullptr && !(ptr->GetFlags() & GC_MARKED)) {
        Object *objptr = nullptr;
        Array *arrayptr = nullptr;

        if ((objptr = ptr->GetPointer<Object>()) != nullptr) {
            int objsize = objptr->GetSize();
            for (int i = 0; i < objsize; i++) {
                objptr->GetMember(i).value.Mark();
            }
        } else if ((arrayptr = ptr->GetPointer<Array>()) != nullptr) {
            int arraysize = arrayptr->GetSize();
            for (int i = 0; i < arraysize; i++) {
                arrayptr->AtIndex(i).Mark();
            }
        }

        ptr->GetFlags() |= GC_MARKED;
    }
}

utf::Utf8String Value::ToString()
{
    const int buf_size = 256;
    char buf[buf_size];
    int n = 0;

    switch (m_type) {
    case Value::I32: {
        n = snprintf(buf, buf_size, "%d", m_value.i32);
        if (n >= buf_size) {
            utf::Utf8String res((size_t)n);
            snprintf(res.GetData(), n, "%d", m_value.i32);
            return res;
        }
        return utf::Utf8String(buf);
    }
    case Value::I64:
        n = snprintf(buf, buf_size, "%" PRId64, m_value.i64);
        if (n >= buf_size) {
            utf::Utf8String res((size_t)n);
            snprintf(res.GetData(), n, "%" PRId64, m_value.i64);
            return res;
        }
        return utf::Utf8String(buf);
    case Value::F32:
        n = snprintf(buf, buf_size, "%g", m_value.f);
        if (n >= buf_size) {
            utf::Utf8String res((size_t)n);
            snprintf(res.GetData(), n, "%g", m_value.f);
            return res;
        }
        return utf::Utf8String(buf);
    case Value::F64:
        n = snprintf(buf, buf_size, "%g", m_value.d);
        if (n >= buf_size) {
            utf::Utf8String res((size_t)n);
            snprintf(res.GetData(), n, "%g", m_value.d);
            return res;
        }
        return utf::Utf8String(buf);
    case Value::BOOLEAN:
        snprintf(buf, buf_size, "%s", m_value.b ? "true" : "false");
        return utf::Utf8String(buf);
    case Value::HEAP_POINTER:
    {
        utf::Utf8String *str = nullptr;
        Object *objptr = nullptr;
        Array *arrayptr = nullptr;
        
        if (m_value.ptr == nullptr) {
            // special case for null pointers
            snprintf(buf, buf_size, "%s", "null");
            return utf::Utf8String(buf);
        } else if ((str = m_value.ptr->GetPointer<utf::Utf8String>()) != nullptr) {
            // return the string directly
            return *str;
        } else if ((arrayptr = m_value.ptr->GetPointer<Array>()) != nullptr) {
            // convert array list to string
            const char sep_str[] = ", ";

            utf::Utf8String res("[", 256);

            // convert all array elements to string
            const int size = arrayptr->GetSize();
            for (int i = 0; i < size; i++) {
                res += arrayptr->AtIndex(i).ToString();

                if (i != size - 1) {
                    res += sep_str;
                }
            }

            res += "]";

            return res;
        } else {
            // return memory address as string
            n = snprintf(buf, buf_size, "%p", (void*)m_value.ptr);
            if (n >= buf_size) {
                utf::Utf8String res((size_t)n);
                snprintf(res.GetData(), n, "%p", (void*)m_value.ptr);
                return res;
            }
            return utf::Utf8String(buf);
        }

        break;
    }
    default: return GetTypeString();
    }
}

} // namespace vm
} // namespace ace