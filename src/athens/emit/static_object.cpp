#include <athens/emit/static_object.hpp>

#include <cstring>

StaticObject::StaticObject()
    : m_id(0),
      m_type(TYPE_NONE)
{
}

StaticObject::StaticObject(int i)
    : m_id(0),
      m_type(TYPE_LABEL)
{
    m_value.lbl = i;
}

StaticObject::StaticObject(const char *str)
    : m_id(0),
      m_type(TYPE_STRING)
{
    int len = std::strlen(str);
    m_value.str = new char[len + 1];
    std::strcpy(m_value.str, str);
}

StaticObject::StaticObject(StaticFunction func)
    : m_id(0),
      m_type(TYPE_FUNCTION)
{
    m_value.func = func;
}

StaticObject::StaticObject(const StaticObject &other)
    : m_id(other.m_id),
      m_type(other.m_type)
{
    if (other.m_type == TYPE_LABEL) {
        m_value.lbl = other.m_value.lbl;
    } else if (other.m_type == TYPE_STRING) {
        int len = std::strlen(other.m_value.str);
        m_value.str = new char[len + 1];
        std::strcpy(m_value.str, other.m_value.str);
    } else if (other.m_type == TYPE_FUNCTION) {
        m_value.func = other.m_value.func;
    }
}

StaticObject::~StaticObject()
{
    if (m_type == TYPE_STRING) {
        delete[] m_value.str;
    }
}

StaticObject &StaticObject::operator=(const StaticObject &other)
{
    if (m_type == TYPE_STRING) {
        delete[] m_value.str;
    }

    m_id = other.m_id;
    m_type = other.m_type;

    if (m_type == TYPE_LABEL) {
        m_value.lbl = other.m_value.lbl;
    } else if (m_type == TYPE_STRING) {
        int len = std::strlen(other.m_value.str);
        m_value.str = new char[len + 1];
        std::strcpy(m_value.str, other.m_value.str);
    } else if (other.m_type == TYPE_FUNCTION) {
        m_value.func = other.m_value.func;
    }

    return *this;
}

bool StaticObject::operator==(const StaticObject &other) const
{
    // do not compare id, we are checking for equality of the values
    if (m_type != other.m_type) {
        return false;
    }

    switch (m_type) {
    case TYPE_LABEL:
        return m_value.lbl == other.m_value.lbl;
        break;
    case TYPE_STRING:
        return !(std::strcmp(m_value.str, other.m_value.str));
        break;
    case TYPE_FUNCTION:
        return m_value.func.m_addr == other.m_value.func.m_addr &&
               m_value.func.m_nargs == other.m_value.func.m_nargs;
        break;
    }
}
