#include <ace-vm/Value.hpp>
#include <ace-vm/Object.hpp>
#include <ace-vm/Array.hpp>
#include <ace-vm/HeapValue.hpp>

#include <common/my_assert.hpp>

#include <stdio.h>
#include <cinttypes>
#include <iostream>

namespace ace {
namespace vm {

/*Value::Value()
    : m_type(HEAP_POINTER)
{
    // initialize to null reference
    m_value.ptr = nullptr;
}*/

Value::Value(const Value &other)
    : m_type(other.m_type),
      m_value(other.m_value)
{
}

void Value::Mark()
{
    HeapValue *ptr = m_value.ptr;
    
    if (m_type == Value::HEAP_POINTER && ptr != nullptr && !(ptr->GetFlags() & GC_MARKED)) {
        if (Object *object = ptr->GetPointer<Object>()) {
            const vm::TypeInfo *type_ptr = object->GetTypePtr();
            ASSERT(type_ptr != nullptr);

            const size_t size = type_ptr->GetSize();
            for (size_t i = 0; i < size; i++) {
                object->GetMember(i).value.Mark();
            }

            // mark the type
            object->GetTypePtrValue().Mark();
        } else if (Array *array = ptr->GetPointer<Array>()) {
            const size_t size = array->GetSize();
            for (int i = 0; i < size; i++) {
                array->AtIndex(i).Mark();
            }
        }

        ptr->GetFlags() |= GC_MARKED;
    }
}

const char *Value::GetTypeString() const
{
    switch (m_type) {
        case I32: // fallthrough
        case I64: return "Int";
        case F32: // fallthrough
        case F64: return "Float";
        case BOOLEAN: return "Boolean";
        case HEAP_POINTER: 
            if (!m_value.ptr) {
                return "Null";
            } else if (m_value.ptr->GetPointer<utf::Utf8String>()) {
                return "String";
            } else if (m_value.ptr->GetPointer<Array>()) {
                return "Array";
            } else if (Object *object = m_value.ptr->GetPointer<Object>()) {
                ASSERT(object->GetTypePtr() != nullptr);
                return object->GetTypePtr()->GetName();
            }
            return "Object";
        case FUNCTION: return "Function";
        case NATIVE_FUNCTION: return "NativeFunction";
        case ADDRESS: return "Address";
        case FUNCTION_CALL: return "FunctionCallInfo";
        case TRY_CATCH_INFO: return "TryCatchInfo";
        default: {
            return "??";
        }
    }
}

utf::Utf8String Value::ToString() const
{
    const int buf_size = 256;
    char buf[buf_size] = {'\0'};
    int n = 0;

    switch (m_type) {
        case Value::I32: {
            n = snprintf(buf, buf_size, "%d", m_value.i32);
            if (n >= buf_size) {
                utf::Utf8String res((size_t)n);
                snprintf(
                    res.GetData(),
                    n,
                    "%d",
                    m_value.i32
                );
                return res;
            }
            return utf::Utf8String(buf);
        }
        case Value::I64:
            n = snprintf(
                buf,
                buf_size,
                "%" PRId64,
                m_value.i64
            );
            if (n >= buf_size) {
                utf::Utf8String res((size_t)n);
                snprintf(
                    res.GetData(),
                    n,
                    "%" PRId64,
                    m_value.i64
                );
                return res;
            }
            return utf::Utf8String(buf);
        case Value::F32:
            n = snprintf(
                buf,
                buf_size,
                "%g",
                m_value.f
            );
            if (n >= buf_size) {
                utf::Utf8String res((size_t)n);
                snprintf(
                    res.GetData(),
                    n,
                    "%g",
                    m_value.f
                );
                return res;
            }
            return utf::Utf8String(buf);
        case Value::F64:
            n = snprintf(
                buf,
                buf_size,
                "%g",
                m_value.d
            );
            if (n >= buf_size) {
                utf::Utf8String res((size_t)n);
                snprintf(
                    res.GetData(),
                    n,
                    "%g",
                    m_value.d
                );
                return res;
            }
            return utf::Utf8String(buf);
        case Value::BOOLEAN:
            return utf::Utf8String(m_value.b ? "true" : "false");
        case Value::HEAP_POINTER:
        {
            if (!m_value.ptr) {
                return utf::Utf8String("null");
            } else if (utf::Utf8String *string = m_value.ptr->GetPointer<utf::Utf8String>()) {
                return *string;
            } else if (Array *array = m_value.ptr->GetPointer<Array>()) {
                utf::Utf8String res(256);
                array->GetRepresentation(res, true);
                return res;
            } else if (Object *object = m_value.ptr->GetPointer<Object>()) {
                utf::Utf8String res;
                object->GetRepresentation(res, true);
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

void Value::ToRepresentation(utf::Utf8String &out_str, bool add_type_name) const
{
    switch (m_type) {
        case Value::HEAP_POINTER:
            if (!m_value.ptr) {
                out_str += "null";
            } else if (utf::Utf8String *string = m_value.ptr->GetPointer<utf::Utf8String>()) {
                out_str += "\"";
                out_str += *string;
                out_str += "\"";
            } else if (Array *array = m_value.ptr->GetPointer<Array>()) {
                array->GetRepresentation(out_str, add_type_name);
            } else if (Object *object = m_value.ptr->GetPointer<Object>()) {
                object->GetRepresentation(out_str, add_type_name);
            } else {
                if (add_type_name) {
                    out_str += GetTypeString();
                    out_str += "(";
                }
                
                out_str += ToString();

                if (add_type_name) {
                    out_str += ")";
                }
            }

            break;
        default:
            out_str += ToString();
    }
}

} // namespace vm
} // namespace ace