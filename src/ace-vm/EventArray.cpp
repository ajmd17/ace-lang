#include <ace-vm/EventArray.hpp>

#include <common/my_assert.hpp>

#include <cstring>

namespace ace {
namespace vm {

EventArray::EventArray(size_t size)
    : m_handlers(new EventHandler[size]),
      m_size(size)
{
}

EventArray::EventArray(const EventArray &other)
    : m_size(other.m_size)
{
    m_handlers = new EventHandler[m_size];

    std::memcpy(
        m_handlers,
        other.m_handlers,
        sizeof(EventHandler) * m_size
    );
}

EventArray::~EventArray()
{
    delete[] m_handlers;
}

EventArray &EventArray::operator=(const EventArray &other)
{
    if (other.m_size > m_size) {
        delete[] m_handlers;
        m_handlers = new EventHandler[other.m_size];
    }

    m_size = other.m_size;

    std::memcpy(
        m_handlers,
        other.m_handlers,
        sizeof(EventHandler) * other.m_size
    );

    return *this;
}

Value *EventArray::Match(const Value &value)
{
    const std::uint32_t value_hash = value.GetHash();

    // used for comparing by values
    union {
        aint64 i;
        afloat64 f;
    } a, b;

    EventHandler *handler;

    for (size_t i = 0; i < m_size; i++) {
        handler = &m_handlers[i];
        ASSERT(handler != nullptr);

        const int match_mode = handler->m_key.m_key_match_mode;

        if (match_mode == MATCH_HASHES) {
            if (handler->m_key.m_hash == value_hash) {
                return &handler->m_handler_func;
            }
        } else if (match_mode == MATCH_VALUES || match_mode == MATCH_TYPES) {
            Value &handler_value = handler->m_key.m_value;

            // compare integers
            if (value.GetInteger(&a.i) && handler_value.GetInteger(&b.i)) {
                if (match_mode == MATCH_TYPES || a.i == b.i) {
                    return &handler->m_handler_func;
                }
            } else if (value.GetNumber(&a.f) && handler_value.GetNumber(&b.f)) {
                if (match_mode == MATCH_TYPES || a.f == b.f) {
                    return &handler->m_handler_func;
                }
            } else if (value.m_type == Value::BOOLEAN && handler_value.m_type == Value::BOOLEAN) {
                if (match_mode == MATCH_TYPES || value.m_value.b == handler_value.m_value.b) {
                    return &handler->m_handler_func;
                }
            } else if (value.m_type == Value::HEAP_POINTER || handler_value.m_type == Value::HEAP_POINTER) {
                HeapValue *hv_a = value.m_value.ptr;
                HeapValue *hv_b = handler_value.m_value.ptr;

                ASSERT(hv_a != nullptr);
                ASSERT(hv_b != nullptr);
                
                if (hv_a->GetTypeId() == hv_b->GetTypeId()) {
                    if (match_mode == MATCH_TYPES || *hv_a == *hv_b) {
                        return &handler->m_handler_func;
                    }
                }
            }
        }
    }

    // no match found
    return nullptr;
}

} // namespace vm
} // namespace ace