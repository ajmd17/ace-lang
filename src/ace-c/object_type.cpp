#include <ace-c/object_type.hpp>
#include <ace-c/ast/ast_expression.hpp>
#include <ace-c/ast/ast_array_expression.hpp>
#include <ace-c/ast/ast_undefined.hpp>
#include <ace-c/ast/ast_void.hpp>
#include <ace-c/ast/ast_null.hpp>
#include <ace-c/ast/ast_false.hpp>
#include <ace-c/ast/ast_integer.hpp>
#include <ace-c/ast/ast_float.hpp>
#include <ace-c/ast/ast_string.hpp>

const ObjectType
    ObjectType::type_builtin_undefined("Undefined", std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof))),
    ObjectType::type_builtin_void("Void", std::shared_ptr<AstVoid>(new AstVoid(SourceLocation::eof))),
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

ObjectType ObjectType::MakeFunctionType(const ObjectType &return_type, const std::vector<ObjectType> &param_types)
{
    std::string type_str = "Function (" + return_type.ToString() + ") ";
    if (!param_types.empty()) {
        for (int i = 0; i < param_types.size(); i++) {
            type_str += "(" + param_types[i].ToString() + ")";
            if (i != param_types.size() - 1) {
                type_str +=  " ";
            }
        }
    } else {
        type_str += "(Void)";
    }

    ObjectType res(type_str, std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof)));
    res.m_is_function = true;
    return res;
}

ObjectType ObjectType::MakeArrayType(const ObjectType &array_member_type)
{
    std::string type_str = "Array (" + array_member_type.ToString() + ") ";
    return ObjectType(type_str, std::shared_ptr<AstArrayExpression>(new AstArrayExpression({}, SourceLocation::eof)));
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
      m_is_function(false)
{
}

ObjectType::ObjectType(const std::string &str,
    const std::shared_ptr<AstExpression> &default_value)
    : m_str(str),
      m_default_value(default_value),
      m_static_id(0),
      m_is_function(false)
{
}

ObjectType::ObjectType(const std::string &str,
    const std::shared_ptr<AstExpression> &default_value,
    const std::vector<DataMember_t> &data_members)
    : m_str(str),
      m_default_value(default_value),
      m_data_members(data_members),
      m_static_id(0),
      m_is_function(false)
{
}

ObjectType::ObjectType(const ObjectType &other)
    : m_str(other.m_str),
      m_default_value(other.m_default_value),
      m_data_members(other.m_data_members),
      m_static_id(other.m_static_id),
      m_is_function(other.m_is_function)
{
}

bool ObjectType::HasDataMember(const std::string &name) const
{
    for (const DataMember_t &dm : m_data_members) {
        if (dm.first == name) {
            return true;
        }
    }
    return false;
}

int ObjectType::GetDataMemberIndex(const std::string &name) const
{
    for (int i = 0; i < m_data_members.size(); i++) {
        if (m_data_members[i].first == name) {
            return i;
        }
    }
    return -1;
}

ObjectType ObjectType::GetDataMemberType(const std::string &name) const
{
    for (const DataMember_t &dm : m_data_members) {
        if (dm.first == name) {
            return dm.second;
        }
    }
    return type_builtin_undefined;
}
