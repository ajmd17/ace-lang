#ifndef STATIC_OBJECT_HPP
#define STATIC_OBJECT_HPP

#include <string>

struct StaticFunction {
    uint32_t m_addr;
    uint8_t m_nargs;
};

struct StaticObject {
    int m_id;

    union {
        int lbl;
        char *str;
        StaticFunction func;
    } m_value;

    enum {
        TYPE_NONE = 0,
        TYPE_LABEL,
        TYPE_STRING,
        TYPE_FUNCTION
    } m_type;

    StaticObject();
    explicit StaticObject(int i);
    explicit StaticObject(const char *str);
    explicit StaticObject(StaticFunction func);
    StaticObject(const StaticObject &other);
    ~StaticObject();

    StaticObject &operator=(const StaticObject &other);
    bool operator==(const StaticObject &other) const;
};

#endif
