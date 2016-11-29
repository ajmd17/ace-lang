#ifndef OBJECT_TYPE_HPP
#define OBJECT_TYPE_HPP

#include <string>
#include <utility>
#include <vector>
#include <memory>

// forward declarations
class ObjectType;
class AstExpression;

typedef std::pair<std::string, ObjectType> DataMember_t;

class ObjectType {
public:
    static const ObjectType
        type_builtin_undefined, // invalid type
        type_builtin_void,
        type_builtin_any,
        type_builtin_bool,
        type_builtin_number,
        type_builtin_int,
        type_builtin_float,
        type_builtin_string;

    static const ObjectType *GetBuiltinType(const std::string &str);
    static ObjectType MakeFunctionType(const ObjectType &return_type, const std::vector<ObjectType> &param_types);
    static ObjectType MakeArrayType(const ObjectType &array_member_type);
    static bool TypeCompatible(const ObjectType &left, const ObjectType &right, bool strict_numbers = false);
    static ObjectType FindCompatibleType(const ObjectType &left, const ObjectType &right, bool use_number = false);

public:
    ObjectType();
    ObjectType(const std::string &str, const std::shared_ptr<AstExpression> &default_value);
    ObjectType(const std::string &str, const std::shared_ptr<AstExpression> &default_value,
        const std::vector<DataMember_t> &data_members);
    ObjectType(const ObjectType &other);
    ~ObjectType();

    inline const std::string &ToString() const { return m_str; }

    inline const std::vector<DataMember_t> &GetDataMembers() const { return m_data_members; }
    bool HasDataMember(const std::string &name) const;
    int GetDataMemberIndex(const std::string &name) const;
    ObjectType GetDataMemberType(const std::string &name) const;
    inline void AddDataMember(const DataMember_t &data_member) { m_data_members.push_back(data_member); }

    inline std::shared_ptr<AstExpression> GetDefaultValue() const { return m_default_value; }
    inline void SetDefaultValue(const std::shared_ptr<AstExpression> &default_value)
        { m_default_value = default_value; }

    inline int GetStaticId() const { return m_static_id; }
    // must be set during type definiton analyzing
    inline void SetStaticId(int static_id) { m_static_id = static_id; }

    // if the objec type is a function type
    inline bool IsFunctionType() const { return m_is_function; }
    inline const std::shared_ptr<ObjectType> &GetReturnType() const { return m_return_type; }
    inline const std::vector<ObjectType> &GetParamTypes() const { return m_param_types; }

    inline bool operator==(const ObjectType &other) const
        { return m_str == other.m_str && m_static_id == other.m_static_id; }
    inline bool operator!=(const ObjectType &other) const { return !operator==(other); }

protected:
    std::string m_str;
    std::shared_ptr<AstExpression> m_default_value;
    std::vector<DataMember_t> m_data_members;
    int m_static_id;

    bool m_is_function;
    std::shared_ptr<ObjectType> m_return_type;
    std::vector<ObjectType> m_param_types;
};

#endif
