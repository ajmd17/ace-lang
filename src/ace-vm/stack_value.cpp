#include <ace-vm/stack_value.hpp>
#include <ace-vm/object.hpp>
#include <ace-vm/array.hpp>

#include <stdio.h>
#include <cinttypes>

StackValue::StackValue()
    : m_type(HEAP_POINTER)
{
    // initialize to null reference
    m_value.ptr = nullptr;
}

StackValue::StackValue(const StackValue &other)
    : m_type(other.m_type),
      m_value(other.m_value)
{
}

void StackValue::Mark()
{
    if (m_type == StackValue::HEAP_POINTER && m_value.ptr != nullptr) {
        Object *objptr = nullptr;
        Array *arrayptr = nullptr;

        if ((objptr = m_value.ptr->GetPointer<Object>()) != nullptr) {
            int objsize = objptr->GetSize();
            for (int i = 0; i < objsize; i++) {
                objptr->GetMember(i).Mark();
            }
        } else if ((arrayptr = m_value.ptr->GetPointer<Array>()) != nullptr) {
            int arraysize = arrayptr->GetSize();
            for (int i = 0; i < arraysize; i++) {
                arrayptr->AtIndex(i).Mark();
            }
        }

        m_value.ptr->GetFlags() |= GC_MARKED;
    }
}

utf::Utf8String StackValue::ToString()
{
    int n = 0;
    const int buf_size = 256;
    char buf[buf_size];

    switch (m_type) {
    case StackValue::INT32: {
        n = snprintf(buf, buf_size, "%d", m_value.i32);
        if (n >= buf_size) {
            utf::Utf8String res((size_t)n);
            snprintf(res.GetData(), n, "%d", m_value.i32);
            return res;
        } else {
            return utf::Utf8String(buf);
        }
        break;
    }
    case StackValue::INT64:
        n = snprintf(buf, buf_size, "%" PRId64, m_value.i64);
        if (n >= buf_size) {
            utf::Utf8String res((size_t)n);
            snprintf(res.GetData(), n, "%" PRId64, m_value.i64);
            return res;
        } else {
            return utf::Utf8String(buf);
        }
        break;
    case StackValue::FLOAT:
        n = snprintf(buf, buf_size, "%g", m_value.f);
        if (n >= buf_size) {
            utf::Utf8String res((size_t)n);
            snprintf(res.GetData(), n, "%g", m_value.f);
            return res;
        } else {
            return utf::Utf8String(buf);
        }
        break;
    case StackValue::DOUBLE:
        n = snprintf(buf, buf_size, "%g", m_value.d);
        if (n >= buf_size) {
            utf::Utf8String res((size_t)n);
            snprintf(res.GetData(), n, "%g", m_value.d);
            return res;
        } else {
            return utf::Utf8String(buf);
        }
        break;
    case StackValue::BOOLEAN:
        n = snprintf(buf, buf_size, "%s", m_value.b ? "true" : "false");
        return utf::Utf8String(buf);
        break;
    case StackValue::HEAP_POINTER:
    {
        utf::Utf8String *str = nullptr;
        Object *objptr = nullptr;
        Array *arrayptr = nullptr;
        
        if (m_value.ptr == nullptr) {
            // special case for null pointers
            n = snprintf(buf, buf_size, "%s", "null");
            return utf::Utf8String(buf);
        } else if ((str = m_value.ptr->GetPointer<utf::Utf8String>()) != nullptr) {
            // return the string directly
            return *str;
        } else if ((arrayptr = m_value.ptr->GetPointer<Array>()) != nullptr) {
            // convert array list to string
            const char sep_str[] = ", ";
            int sep_str_len = std::strlen(sep_str);

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
            } else {
                return utf::Utf8String(buf);
            }
        }

        break;
    }
    default:
        return GetTypeString();
    }
}

/*void StackValue::ToString(char dst[], int maxlength)
{
    int n = 0;
    switch (m_type) {
    case StackValue::INT32:
        n = snprintf(dst, maxlength, "%d", m_value.i32);
        break;
    case StackValue::INT64:
        n = snprintf(dst, maxlength, "%" PRId64, m_value.i64);
        break;
    case StackValue::FLOAT:
        n = snprintf(dst, maxlength, "%g", m_value.f);
        break;
    case StackValue::DOUBLE:
        n = snprintf(dst, maxlength, "%g", m_value.d);
        break;
    case StackValue::BOOLEAN:
        n = snprintf(dst, maxlength, m_value.b ? "true" : "false");
        break;
    case StackValue::HEAP_POINTER:
    {
        utf::Utf8String *str = nullptr;
        Object *objptr = nullptr;
        Array *arrayptr = nullptr;
        if (m_value.ptr == nullptr) {
            // special case for null pointers
            n = snprintf(dst, maxlength, "null");
        } else if ((str = m_value.ptr->GetPointer<utf::Utf8String>()) != nullptr) {
            // print string value
            n = snprintf(dst, maxlength, "%s", str->GetData());
        } else if ((arrayptr = m_value.ptr->GetPointer<Array>()) != nullptr) {
            const int buffer_size = 256;
            char buffer[buffer_size] = {'\0'};
            int buffer_index = 0;
            buffer[buffer_index++] = '[';

            const char sep_str[] = ", ";
            int sep_str_len = std::strlen(sep_str);

            // convert all array elements to string
            const int size = arrayptr->GetSize();
            for (int i = 0; i < size; i++) {
                char tmp[buffer_size] = {'\0'};
                bool last = i != size - 1;

                arrayptr->AtIndex(i).ToString(tmp, buffer_size - (!last ? sep_str_len : 0));

                int len = std::strlen(tmp);
                if (last) {
                    len += sep_str_len;
                    std::strcat(tmp, sep_str);
                }

                if (buffer_index + len < buffer_size - 5) {
                    std::strcat(buffer, tmp);
                    buffer_index += len;
                } else {
                    std::strcat(buffer, "... ");
                    break;
                }
            }

            std::strcat(buffer, "]");

            n = snprintf(dst, maxlength, "%s", buffer);
        } else {
            n = snprintf(dst, maxlength, "Object<%p>", (void*)m_value.ptr);
        }

        break;
    }
    case StackValue::FUNCTION:
        n = snprintf(dst, maxlength, "Function<%du>", m_value.func.m_addr);
        break;
    case StackValue::NATIVE_FUNCTION:
        n = snprintf(dst, maxlength, "NativeFunction<%p>", (void*)m_value.native_func);
        break;
    case StackValue::ADDRESS:
        n = snprintf(dst, maxlength, "Address<%du>", m_value.addr);
        break;
    case StackValue::TYPE_INFO:
        n = snprintf(dst, maxlength, "Type<%du>", m_value.type_info.m_size);
        break;
    }
}*/