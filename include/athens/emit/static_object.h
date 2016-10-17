#ifndef STATIC_OBJECT_H
#define STATIC_OBJECT_H

#include <string>

struct StaticObject {
    int m_id;

    union {
        int lbl;
        char *str;
    } m_value;

    enum {
        TYPE_NONE = 0,
        TYPE_LABEL,
        TYPE_STRING,
    } m_type;

    StaticObject();
    explicit StaticObject(int i);
    explicit StaticObject(const char *str);
    StaticObject(const StaticObject &other);
    ~StaticObject();

    StaticObject &operator=(const StaticObject &other);
    bool operator==(const StaticObject &other) const;
};

#endif