#include <ace-c/emit/StaticObject.hpp>
#include <common/my_assert.hpp>

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

StaticObject::StaticObject(const StaticFunction &func)
    : m_id(0),
      m_type(TYPE_FUNCTION)
{
    m_value.func = func;
}

StaticObject::StaticObject(const StaticTypeInfo &type_info)
    : m_id(0),
      m_type(TYPE_TYPE_INFO)
{
    ASSERT_MSG(
        type_info.m_names.size() == type_info.m_size,
        "number of names must be equal to the number of members"
    );

    int len = std::strlen(type_info.m_name);
    m_value.type_info.m_name = new char[len + 1];
    std::strcpy(m_value.type_info.m_name, type_info.m_name);
    m_value.type_info.m_size = type_info.m_size;
    m_value.type_info.m_names = type_info.m_names;
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
    } else if (other.m_type == TYPE_TYPE_INFO) {
        m_value.type_info.m_names = other.m_value.type_info.m_names;
        m_value.type_info.m_size = other.m_value.type_info.m_size;
        int len = std::strlen(other.m_value.type_info.m_name);
        m_value.type_info.m_name = new char[len + 1];
        std::strcpy(m_value.type_info.m_name, other.m_value.type_info.m_name);
    }
}

StaticObject::~StaticObject()
{
    if (m_type == TYPE_STRING) {
        delete[] m_value.str;
    } else if (m_type == TYPE_TYPE_INFO) {
        delete[] m_value.type_info.m_name;
    }
}

StaticObject &StaticObject::operator=(const StaticObject &other)
{
    if (m_type == TYPE_STRING) {
        delete[] m_value.str;
    } else if (m_type == TYPE_TYPE_INFO) {
        delete[] m_value.type_info.m_name;
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
    } else if (other.m_type == TYPE_TYPE_INFO) {
        m_value.type_info.m_names = other.m_value.type_info.m_names;
        m_value.type_info.m_size = other.m_value.type_info.m_size;
        int len = std::strlen(other.m_value.type_info.m_name);
        m_value.type_info.m_name = new char[len + 1];
        std::strcpy(m_value.type_info.m_name, other.m_value.type_info.m_name);
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
        return m_value.func.m_addr == other.m_value.func.m_addr;
        break;
    case TYPE_TYPE_INFO:
        return !(std::strcmp(m_value.type_info.m_name, other.m_value.type_info.m_name)) &&
                m_value.type_info.m_size == other.m_value.type_info.m_size;
        break;
    default:
        return false;
    }
}
