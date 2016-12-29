#if 0

#include <ace-c/ObjectType.hpp>
#include <ace-c/ast/AstTypeContract.hpp>
#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/ast/AstArrayExpression.hpp>
#include <ace-c/ast/AstUndefined.hpp>
#include <ace-c/ast/AstNull.hpp>
#include <ace-c/ast/AstFalse.hpp>
#include <ace-c/ast/AstInteger.hpp>
#include <ace-c/ast/AstFloat.hpp>
#include <ace-c/ast/AstString.hpp>

#include <common/my_assert.hpp>

DataMember::DataMember(const std::string &name, const ObjectType &type)
    : m_name(name),
      m_type(type)
{
}

DataMember::DataMember(const DataMember &other)
    : m_name(other.m_name),
      m_type(other.m_type)
{
}

const ObjectType
    ObjectType::type_builtin_undefined("Undefined", std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof))),
    ObjectType::type_builtin_any("Any", std::shared_ptr<AstNull>(new AstNull(SourceLocation::eof))),
    ObjectType::type_builtin_bool("Bool", std::shared_ptr<AstFalse>(new AstFalse(SourceLocation::eof))),
    ObjectType::type_builtin_number("Number", std::shared_ptr<AstInteger>(new AstInteger(0, SourceLocation::eof))),
    ObjectType::type_builtin_int("Int", std::shared_ptr<AstInteger>(new AstInteger(0, SourceLocation::eof))),
    ObjectType::type_builtin_float("Float", std::shared_ptr<AstFloat>(new AstFloat(0.0, SourceLocation::eof))),
    ObjectType::type_builtin_string("String", std::shared_ptr<AstString>(new AstString("", SourceLocation::eof)));

const ObjectType *ObjectType::GetBuiltinType(const std::string &str)
{
    if (str == type_builtin_undefined.ToString()) {
        return &type_builtin_undefined;
    } else if (str == type_builtin_void.ToString()) {
        return &type_builtin_void;
    } else if (str == type_builtin_any.ToString()) {
        return &type_builtin_any;
    } else if (str == type_builtin_bool.ToString()) {
        return &type_builtin_bool;
    } else if (str == type_builtin_number.ToString()) {
        return &type_builtin_number;
    } else if (str == type_builtin_int.ToString()) {
        return &type_builtin_int;
    } else if (str == type_builtin_float.ToString()) {
        return &type_builtin_float;
    } else if (str == type_builtin_string.ToString()) {
        return &type_builtin_string;
    }

    return nullptr;
}

ObjectType ObjectType::MakeGenericType(const ObjectType &base_type, const std::vector<ObjectType> &param_types)
{
    std::string type_str = base_type.ToString() + " ";

    if (!param_types.empty()) {
        for (int i = 0; i < param_types.size(); i++) {
            type_str += "(" + param_types[i].ToString() + ")";
            if (i != param_types.size() - 1) {
                type_str += " ";
            }
        }
    }

    ObjectType res(type_str, std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof)));
    res.m_is_generic = true;
    res.m_param_types = param_types;
    return res;
}

ObjectType ObjectType::MakeFunctionType(const ObjectType &return_type, const std::vector<ObjectType> &param_types)
{
    std::string type_str = "Function ";
    if (!param_types.empty()) {
        for (int i = 0; i < param_types.size(); i++) {
            type_str += "(" + param_types[i].ToString() + ")";
            if (i != param_types.size() - 1) {
                type_str +=  " ";
            }
        }
    } else {
        type_str += "(None)";
    }
    type_str += " -> " + return_type.ToString();

    ObjectType res(type_str, std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof)));
    res.m_is_function = true;
    res.m_return_type.reset(new ObjectType(return_type));
    res.m_param_types = param_types;
    return res;
}

ObjectType ObjectType::MakeArrayType(const ObjectType &array_held_type)
{
    std::string type_str = "Array (" + array_held_type.ToString() + ")";

    ObjectType res(type_str, std::shared_ptr<AstArrayExpression>(new AstArrayExpression({}, SourceLocation::eof)));
    res.m_is_array = true;
    res.m_array_held_type.reset(new ObjectType(array_held_type));
    return res;
}

bool ObjectType::TypeCompatible(const ObjectType &left, const ObjectType &right, bool strict_numbers)
{
    if (right == type_builtin_undefined) {
        // nothing is compatible with Undefined!
        return false;
    } else if (left == right || (left == type_builtin_any || right == type_builtin_any)) {
        return true;
    } else if (left == type_builtin_number) {
        // if the type Number is specified, it could be either Int or Float.
        return right == type_builtin_int ||
               right == type_builtin_float;
    } else if ((!strict_numbers) && (left == type_builtin_int ||
                                     left == type_builtin_float)) {

        return right == type_builtin_number ||
               right == type_builtin_float  ||
               right == type_builtin_int;
    }

    return false;
}

ObjectType ObjectType::FindCompatibleType(const ObjectType &left, const ObjectType &right, bool use_number)
{
    std::string left_str = left.ToString();
    std::string right_str = right.ToString();

    if (left_str == right_str) {
        return left;
    } else if (left_str == type_builtin_undefined.ToString() ||
               right_str == type_builtin_undefined.ToString()) {
        return type_builtin_undefined;
    } else if (left_str == type_builtin_any.ToString()) {
        // Any + <any type> = Any
        return left;
    } else if (right_str == type_builtin_any.ToString()) {
        // <any type> + Any = <original type>
        return left;
    }  else if (left_str == type_builtin_number.ToString()) {
        return (right_str == type_builtin_int.ToString() ||
                right_str == type_builtin_float.ToString()) ? (use_number ? type_builtin_number : left) : type_builtin_undefined;
    } else if (left_str == type_builtin_int.ToString()) {
        return (right_str == type_builtin_number.ToString() ||
                right_str == type_builtin_float.ToString()) ? (use_number ? type_builtin_number : right) : type_builtin_undefined;
    } else if (left_str == type_builtin_float.ToString()) {
        return (right_str == type_builtin_number.ToString() ||
                right_str == type_builtin_int.ToString()) ? (use_number ? type_builtin_number : left) : type_builtin_undefined;
    } else if (right_str == type_builtin_number.ToString()) {
        return (left_str == type_builtin_int.ToString() ||
                left_str == type_builtin_float.ToString()) ? (use_number ? type_builtin_number : right) : type_builtin_undefined;
    } else if (right_str == type_builtin_int.ToString()) {
        return (left_str == type_builtin_number.ToString() ||
                left_str == type_builtin_float.ToString()) ? (use_number ? type_builtin_number : left) : type_builtin_undefined;
    } else if (right_str == type_builtin_float.ToString()) {
        return (left_str == type_builtin_number.ToString() ||
                left_str == type_builtin_int.ToString()) ? (use_number ? type_builtin_number : right) : type_builtin_undefined;
    }

    return type_builtin_undefined;
}

ObjectType::ObjectType()
    : m_str("Any"),
      m_default_value(std::shared_ptr<AstNull>(
          new AstNull(SourceLocation::eof))),
      m_static_id(0),
      m_is_generic(false),
      m_is_function(false),
      m_return_type(nullptr)
{
}

ObjectType::ObjectType(const std::string &str,
    const std::shared_ptr<AstExpression> &default_value)
    : m_str(str),
      m_default_value(default_value),
      m_static_id(0),
      m_is_function(false),
      m_return_type(nullptr),
      m_type_contract(nullptr)
{
}

ObjectType::ObjectType(const std::string &str,
    const std::shared_ptr<AstExpression> &default_value,
    const std::vector<DataMember> &data_members)
    : m_str(str),
      m_default_value(default_value),
      m_data_members(data_members),
      m_static_id(0),
      m_is_function(false),
      m_return_type(nullptr),
      m_type_contract(nullptr)
{
}

ObjectType::ObjectType(const std::string &str,
    const std::shared_ptr<AstExpression> &default_value,
    const std::vector<DataMember> &data_members,
    const std::shared_ptr<AstTypeContractExpression> &type_contract)
    : m_str(str),
      m_default_value(default_value),
      m_data_members(data_members),
      m_static_id(0),
      m_is_function(false),
      m_return_type(nullptr),
      m_type_contract(type_contract)
{
}

ObjectType::ObjectType(const ObjectType &other)
    : m_str(other.m_str),
      m_default_value(other.m_default_value),
      m_data_members(other.m_data_members),
      m_static_id(other.m_static_id),
      m_is_function(other.m_is_function),
      m_return_type(other.m_return_type != nullptr 
        ? std::shared_ptr<ObjectType>(new ObjectType(*other.m_return_type.get()))
        : nullptr),
      m_param_types(other.m_param_types),
      m_type_contract(other.m_type_contract)
{
}

ObjectType::~ObjectType()
{
}

bool ObjectType::operator==(const ObjectType &other) const
{
    if (m_str != other.m_str) {
        return false;
    }

    if (!(m_is_generic == other.m_is_generic &&
        m_is_function == other.m_is_function &&
        m_is_array == other.m_is_array &&
        m_static_id == other.m_static_id)) {
        return false;
    }

    for (const auto &i : m_data_members) {
        for (const auto &j : other.m_data_members) {
            if (i.m_name != j.m_name || i.m_type != j.m_type) {
                return false;
            }
        }
    }

    if (m_is_generic || m_is_function) {
        if (m_param_types.size() != other.m_param_types.size()) {
            return false;
        }

        for (const auto &i : m_param_types) {
            for (const auto &j : other.m_param_types) {
                if (i != j) {
                    return false;
                }
            }
        }
    } else if (m_is_array) {
        ASSERT(m_array_held_type != nullptr);
        ASSERT(other.m_array_held_type != nullptr);
        if ((*m_array_held_type) != (*other.m_array_held_type)) {
            return false;
        }
    }

    return true;
}

bool ObjectType::HasDataMember(const std::string &name) const
{
    for (const DataMember &dm : m_data_members) {
        if (dm.m_name == name) {
            return true;
        }
    }
    return false;
}

int ObjectType::GetDataMemberIndex(const std::string &name) const
{
    for (int i = 0; i < m_data_members.size(); i++) {
        if (m_data_members[i].m_name == name) {
            return i;
        }
    }
    return -1;
}

ObjectType ObjectType::GetDataMemberType(const std::string &name) const
{
    for (const DataMember &dm : m_data_members) {
        if (dm.m_name == name) {
            return dm.m_type;
        }
    }
    return type_builtin_undefined;
}

bool ObjectType::IsRecordType() const
{
    if (m_is_function || (*this == type_builtin_any)) {
        return false;
    }
    
    for (const DataMember &dm : m_data_members) {
        if (!dm.m_type.IsRecordType()) {
            return false;
        }
    }

    return true;
}

#endif