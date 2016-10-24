#include <athens/object_type.hpp>
#include <athens/ast/ast_expression.hpp>
#include <athens/ast/ast_void.hpp>
#include <athens/ast/ast_null.hpp>
#include <athens/ast/ast_false.hpp>
#include <athens/ast/ast_integer.hpp>
#include <athens/ast/ast_float.hpp>
#include <athens/ast/ast_string.hpp>

const ObjectType
    ObjectType::type_builtin_void("Void", std::shared_ptr<AstVoid>(new AstVoid(SourceLocation::eof))),
    ObjectType::type_builtin_any("Any", std::shared_ptr<AstNull>(new AstNull(SourceLocation::eof))),
    ObjectType::type_builtin_bool("Bool", std::shared_ptr<AstFalse>(new AstFalse(SourceLocation::eof))),
    ObjectType::type_builtin_number("Number", std::shared_ptr<AstInteger>(new AstInteger(0, SourceLocation::eof))),
    ObjectType::type_builtin_int("Int", std::shared_ptr<AstInteger>(new AstInteger(0, SourceLocation::eof))),
    ObjectType::type_builtin_float("Float", std::shared_ptr<AstFloat>(new AstFloat(0.0, SourceLocation::eof))),
    ObjectType::type_builtin_string("String", std::shared_ptr<AstString>(new AstString("", SourceLocation::eof)));

const ObjectType *ObjectType::GetBuiltinType(const std::string &str)
{
    if (str == type_builtin_void.ToString()) {
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

ObjectType::ObjectType()
    : m_str("Any"),
      m_default_value(std::shared_ptr<AstNull>(
          new AstNull(SourceLocation::eof)))
{
}

ObjectType::ObjectType(const std::string &str,
    std::shared_ptr<AstExpression> default_value)
    : m_str(str),
      m_default_value(default_value)
{
}

ObjectType::ObjectType(const std::string &str,
    std::shared_ptr<AstExpression> default_value,
    const std::vector<DataMember_t> &data_members)
    : m_str(str),
      m_default_value(default_value),
      m_data_members(data_members)
{
}

ObjectType::ObjectType(const ObjectType &other)
    : m_str(other.m_str),
      m_default_value(other.m_default_value),
      m_data_members(other.m_data_members)
{
}

bool ObjectType::TypeCompatible(const ObjectType &left, const ObjectType &right, bool strict_numbers) {
    std::string left_str = left.ToString();
    std::string right_str = right.ToString();

    if (left_str == right_str || left_str == type_builtin_any.ToString()) {
        return true;
    } else if (left_str == type_builtin_number.ToString()) {
        // if the type Number is specified, it could be either Int or Float.
        return right_str == type_builtin_int.ToString() ||
               right_str == type_builtin_float.ToString();
    } else if (!strict_numbers && (left_str == type_builtin_int.ToString() ||
                                   left_str == type_builtin_float.ToString())) {

        return right_str == type_builtin_number.ToString() ||
               right_str == type_builtin_float.ToString();
    }

    return false;
}

ObjectType ObjectType::FindCompatibleType(const ObjectType &left, const ObjectType &right)
{
    std::string left_str = left.ToString();
    std::string right_str = right.ToString();

    if (left_str == right_str) {
        return left;
    } else if (left_str == type_builtin_void.ToString() ||
               right_str == type_builtin_void.ToString()) {
        return type_builtin_void;
    } else if (left_str == type_builtin_any.ToString() ||
               right_str == type_builtin_any.ToString()) {
        return type_builtin_any;
    } else if (left_str == type_builtin_number.ToString()) {
        return (right_str == type_builtin_int.ToString() ||
                right_str == type_builtin_float.ToString()) ? left : type_builtin_void;
    } else if (left_str == type_builtin_int.ToString()) {
        return (right_str == type_builtin_number.ToString() ||
                right_str == type_builtin_float.ToString()) ? right : type_builtin_void;
    } else if (left_str == type_builtin_float.ToString()) {
        return (right_str == type_builtin_number.ToString() ||
                right_str == type_builtin_int.ToString()) ? left : type_builtin_void;
    } else if (right_str == type_builtin_number.ToString()) {
        return (left_str == type_builtin_int.ToString() ||
                left_str == type_builtin_float.ToString()) ? right : type_builtin_void;
    } else if (right_str == type_builtin_int.ToString()) {
        return (left_str == type_builtin_number.ToString() ||
                left_str == type_builtin_float.ToString()) ? left : type_builtin_void;
    } else if (right_str == type_builtin_float.ToString()) {
        return (left_str == type_builtin_number.ToString() ||
                left_str == type_builtin_int.ToString()) ? right : type_builtin_void;
    }

    return type_builtin_void;
}
