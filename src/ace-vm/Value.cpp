#include <ace-vm/Value.hpp>
#include <ace-vm/Object.hpp>
#include <ace-vm/Array.hpp>
#include <ace-vm/ImmutableString.hpp>
#include <ace-vm/HeapValue.hpp>

#include <common/my_assert.hpp>

#include <stdio.h>
#include <cinttypes>
#include <iostream>

namespace ace {
namespace vm {

static const ImmutableString NULL_STRING = ImmutableString("null");
static const ImmutableString BOOLEAN_STRINGS[2] = {
    ImmutableString("false"),
    ImmutableString("true")
};

Value::Value(const Value &other)
    : m_type(other.m_type),
      m_value(other.m_value)
{
}

void Value::Mark()
{
    switch (m_type) {
        case Value::VALUE_REF:
            ASSERT(m_value.value_ref != nullptr);
            m_value.value_ref->Mark();
            break;
        
        case Value::HEAP_POINTER: {
            HeapValue *ptr = m_value.ptr;
            if (ptr != nullptr && !(ptr->GetFlags() & GC_MARKED)) {
                if (Object *object = ptr->GetPointer<Object>()) {
                    HeapValue *proto = nullptr;

                    do {
                        const size_t size = object->GetSize();

                        for (size_t i = 0; i < size; i++) {
                            object->GetMember(i).value.Mark();
                        }

                        proto = object->GetPrototype();

                        if (proto != nullptr) {
                            proto->GetFlags() |= GC_MARKED;
                            object = proto->GetPointer<Object>();
                        }
                    } while (proto != nullptr);
                } else if (Array *array = ptr->GetPointer<Array>()) {
                    const size_t size = array->GetSize();
                    for (int i = 0; i < size; i++) {
                        array->AtIndex(i).Mark();
                    }
                }

                ptr->GetFlags() |= GC_MARKED;
            }
        }

        default: break;
    }
}

const char *Value::GetTypeString() const
{
    switch (m_type) {
        case I32:     // fallthrough
        case I64:     return "Int";
        case F32:     // fallthrough
        case F64:     return "Float";
        case BOOLEAN: return "Boolean";
        case VALUE_REF:
            ASSERT(m_value.value_ref != nullptr);
            return m_value.value_ref->GetTypeString();

        case HEAP_POINTER: 
            if (m_value.ptr == nullptr) {
                return "Null";
            } else if (m_value.ptr->GetPointer<ImmutableString>()) {
                return "String";
            } else if (m_value.ptr->GetPointer<Array>()) {
                return "Array";
            } else if (Object *object = m_value.ptr->GetPointer<Object>()) {
                return "Object"; // TODO prototype name
            }

            return "Object";

        case FUNCTION:        return "Function";
        case NATIVE_FUNCTION: return "NativeFunction";
        case ADDRESS:         return "Address";
        case FUNCTION_CALL:   return "FunctionCallInfo";
        case TRY_CATCH_INFO:  return "TryCatchInfo";
        case USER_DATA:       return "UserData";
        default:              return "??";
    }
}

ImmutableString Value::ToString() const
{
    const size_t buf_size = 256;
    char buf[buf_size] = {0};

    switch (m_type) {
        case Value::I32: {
            int n = snprintf(buf, buf_size, "%d", m_value.i32);
            return ImmutableString(buf, n);
        }

        case Value::I64: {
            int n = snprintf(
                buf,
                buf_size,
                "%" PRId64,
                m_value.i64
            );
            return ImmutableString(buf, n);
        }

        case Value::F32: {
            int n = snprintf(
                buf,
                buf_size,
                "%g",
                m_value.f
            );
            return ImmutableString(buf, n);
        }

        case Value::F64: {
            int n = snprintf(
                buf,
                buf_size,
                "%g",
                m_value.d
            );
            return ImmutableString(buf, n);
        }

        case Value::BOOLEAN:
            return BOOLEAN_STRINGS[m_value.b];

        case Value::VALUE_REF:
            ASSERT(m_value.value_ref != nullptr);
            return m_value.value_ref->ToString();

        case Value::HEAP_POINTER: {
            if (m_value.ptr == nullptr) {
                return NULL_STRING;
            } else if (ImmutableString *string = m_value.ptr->GetPointer<ImmutableString>()) {
                return *string;
            } else if (Array *array = m_value.ptr->GetPointer<Array>()) {
                std::stringstream ss;
                array->GetRepresentation(ss, true);
                const std::string &str = ss.str();
                return ImmutableString(str.c_str());
            } else if (Object *object = m_value.ptr->GetPointer<Object>()) {
                std::stringstream ss;
                object->GetRepresentation(ss, true);
                const std::string &str = ss.str();
                return ImmutableString(str.c_str());
            } else {
                // return memory address as string
                int n = snprintf(buf, buf_size, "%p", (void*)m_value.ptr);
                return ImmutableString(buf, n);
            }

            break;
        }

        default: return ImmutableString(GetTypeString());
    }
}

void Value::ToRepresentation(std::stringstream &ss, bool add_type_name) const
{
    switch (m_type) {
        case Value::VALUE_REF:
            ASSERT(m_value.value_ref != nullptr);
            m_value.value_ref->ToRepresentation(ss, add_type_name);
            return;

        case Value::HEAP_POINTER:
            if (m_value.ptr == nullptr) {
                ss << "null";
            } else if (ImmutableString *string = m_value.ptr->GetPointer<ImmutableString>()) {
                ss << '\"';
                ss << string->GetData();
                ss << '\"';
            } else if (Array *array = m_value.ptr->GetPointer<Array>()) {
                array->GetRepresentation(ss, add_type_name);
            } else if (Object *object = m_value.ptr->GetPointer<Object>()) {
                object->GetRepresentation(ss, add_type_name);
            } else {
                if (add_type_name) {
                    ss << GetTypeString();
                    ss << '(';
                }
                
                ss << ToString().GetData();

                if (add_type_name) {
                    ss << ')';
                }
            }

            break;
        default:
            ss << ToString().GetData();
    }
}

} // namespace vm
} // namespace ace