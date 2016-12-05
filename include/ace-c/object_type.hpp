#ifndef OBJECT_TYPE_HPP
#define OBJECT_TYPE_HPP

#include <string>
#include <utility>
#include <vector>
#include <memory>

// forward declarations
class ObjectType;
class AstExpression;
class AstTypeContractExpression;
struct DataMember;

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
        const std::vector<DataMember> &data_members);
    ObjectType(const std::string &str, const std::shared_ptr<AstExpression> &default_value,
        const std::vector<DataMember> &data_members,
        const std::shared_ptr<AstTypeContractExpression> &type_contract);
    ObjectType(const ObjectType &other);
    ~ObjectType();

    inline const std::string &ToString() const { return m_str; }

    inline const DataMember &GetDataMember(int index) const { return m_data_members[index]; }
    inline const std::vector<DataMember> &GetDataMembers() const { return m_data_members; }
    bool HasDataMember(const std::string &name) const;
    int GetDataMemberIndex(const std::string &name) const;
    ObjectType GetDataMemberType(const std::string &name) const;
    inline void AddDataMember(const DataMember &data_member) { m_data_members.push_back(data_member); }

    inline std::shared_ptr<AstExpression> GetDefaultValue() const { return m_default_value; }
    inline void SetDefaultValue(const std::shared_ptr<AstExpression> &default_value) { m_default_value = default_value; }

    inline int GetStaticId() const { return m_static_id; }
    // must be set during type definiton analyzing
    inline void SetStaticId(int static_id) { m_static_id = static_id; }

    /** Checks if the object type is a record type (no functions or nested objects with functions).Accept
        Also, 'Any' type means that this cannot be determined therefore it will return false.
    */
    bool IsRecordType() const;

    /** Checks if the object type is a function type **/
    inline bool IsFunctionType() const { return m_is_function; }
    /** Gets the return type of this object type (if it is a function) */
    inline const std::shared_ptr<ObjectType> &GetReturnType() const { return m_return_type; }
    /** Gets the parameter types of this object type (if it is a function) */
    inline const std::vector<ObjectType> &GetParamTypes() const { return m_param_types; }

    inline bool HasTypeContract() const { return m_type_contract != nullptr; }
    inline const std::shared_ptr<AstTypeContractExpression> &GetTypeContract() const { return m_type_contract; }
    inline void SetTypeContract(const std::shared_ptr<AstTypeContractExpression> &type_contract)
        { m_type_contract = type_contract; }

    inline bool operator==(const ObjectType &other) const
        { return m_str == other.m_str && m_static_id == other.m_static_id; }
    inline bool operator!=(const ObjectType &other) const { return !operator==(other); }

protected:
    std::string m_str;
    std::shared_ptr<AstExpression> m_default_value;
    std::vector<DataMember> m_data_members;
    int m_static_id;

    bool m_is_function;
    std::shared_ptr<ObjectType> m_return_type;
    std::vector<ObjectType> m_param_types;

    // if it has a type contract
    std::shared_ptr<AstTypeContractExpression> m_type_contract;
};

struct DataMember {
    std::string m_name;
    ObjectType m_type;

    DataMember(const std::string &name, const ObjectType &type);
    DataMember(const DataMember &other);
};

#endif
