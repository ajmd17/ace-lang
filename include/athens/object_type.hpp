#ifndef OBJECT_TYPE_HPP
#define OBJECT_TYPE_HPP

#include <string>
#include <utility>
#include <vector>

// forward declaration
class ObjectType;

typedef std::pair<std::string, ObjectType> DataMember_t;

class ObjectType {
public:
    static const ObjectType
        type_builtin_void, // invalid type
        type_builtin_any,
        type_builtin_bool,
        type_builtin_number,
        type_builtin_int,
        type_builtin_float,
        type_builtin_string;

    static const ObjectType *GetBuiltinType(const std::string &str);

public:
    ObjectType();
    ObjectType(const std::string &str);
    ObjectType(const std::string &str, const std::vector<DataMember_t> &data_members);
    ObjectType(const ObjectType &other);

    inline const std::string &ToString() const { return m_str; }

    inline const std::vector<DataMember_t> &GetDataMembers() const { return m_data_members; }
    bool HasDataMember(const std::string &name) const;
    const ObjectType &GetDataMemberType(const std::string &name) const;
    void AddDataMember(const DataMember_t &data_member);

    static bool TypeCompatible(const ObjectType &left, const ObjectType &right, bool strict_numbers = false);
    static ObjectType FindCompatibleType(const ObjectType &left, const ObjectType &right);

protected:
    std::string m_str;
    std::vector<DataMember_t> m_data_members;
};

#endif
