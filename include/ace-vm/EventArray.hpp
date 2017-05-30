#ifndef EVENT_ARRAY_HPP
#define EVENT_ARRAY_HPP

#include <ace-vm/Value.hpp>

#include <stdint.h>

namespace ace {
namespace vm {

enum KeyMatchMode : uint8_t {
    MATCH_HASHES,
    MATCH_VALUES,
    MATCH_TYPES,
};

struct EventHandler {
    struct {
        KeyMatchMode m_key_match_mode;

        union {
            uint32_t m_hash;
            Value m_value;
        };
    } m_key;

    Value m_handler_func;
};

class EventArray {
public:
    EventArray(size_t size);
    EventArray(const EventArray &other);
    ~EventArray();

    EventArray &operator=(const EventArray &other);
    
    // compare by memory address
    inline bool operator==(const EventArray &other) const { return this == &other; }

    inline const EventHandler *GetHandlers() const
        { return m_handlers; }

    Value *Match(const Value &value);

private:
    EventHandler *m_handlers;
    size_t m_size;
};

} // namespace vm
} // namespace ace

#endif