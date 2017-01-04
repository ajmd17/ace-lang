#include <ace-c/SymbolType.hpp>
#include <ace-c/ast/AstStatement.hpp>
#include <ace-c/ast/AstUndefined.hpp>
#include <ace-c/ast/AstNil.hpp>
#include <ace-c/ast/AstInteger.hpp>
#include <ace-c/ast/AstFloat.hpp>
#include <ace-c/ast/AstFalse.hpp>
#include <ace-c/ast/AstString.hpp>
#include <ace-c/ast/AstFunctionExpression.hpp>
#include <ace-c/ast/AstArrayExpression.hpp>
#include <ace-c/ast/AstObject.hpp>

#include <common/my_assert.hpp>
#include <common/utf8.hpp>

const SymbolTypePtr_t SymbolType::Builtin::UNDEFINED = SymbolType::Primitive("Undefined", std::shared_ptr<AstUndefined>(new AstUndefined(SourceLocation::eof)));
const SymbolTypePtr_t SymbolType::Builtin::ANY       = SymbolType::Primitive("Any", std::shared_ptr<AstNil>(new AstNil(SourceLocation::eof)));
const SymbolTypePtr_t SymbolType::Builtin::OBJECT    = SymbolType::Primitive("Object", nullptr, nullptr);
const SymbolTypePtr_t SymbolType::Builtin::INT       = SymbolType::Primitive("Int", std::shared_ptr<AstInteger>(new AstInteger(0, SourceLocation::eof)));
const SymbolTypePtr_t SymbolType::Builtin::FLOAT     = SymbolType::Primitive("Float", std::shared_ptr<AstFloat>(new AstFloat(0.0, SourceLocation::eof)));
const SymbolTypePtr_t SymbolType::Builtin::NUMBER    = SymbolType::Primitive("Number", std::shared_ptr<AstInteger>(new AstInteger(0, SourceLocation::eof)));
const SymbolTypePtr_t SymbolType::Builtin::BOOLEAN   = SymbolType::Primitive("Boolean", std::shared_ptr<AstFalse>(new AstFalse(SourceLocation::eof)));
const SymbolTypePtr_t SymbolType::Builtin::STRING    = SymbolType::Primitive("String", std::shared_ptr<AstString>(new AstString("", SourceLocation::eof)));
const SymbolTypePtr_t SymbolType::Builtin::FUNCTION  = SymbolType::Generic("Function", std::shared_ptr<AstFunctionExpression>(new AstFunctionExpression({}, nullptr, 
    std::shared_ptr<AstBlock>(new AstBlock(SourceLocation::eof)), SourceLocation::eof)), {}, GenericTypeInfo{ -1 });
const SymbolTypePtr_t SymbolType::Builtin::ARRAY     = SymbolType::Generic("Array", std::shared_ptr<AstArrayExpression>(new AstArrayExpression({}, SourceLocation::eof)), {}, GenericTypeInfo{ 1 });

SymbolType::SymbolType(const std::string &name, 
    SymbolTypeClass type_class, 
    const SymbolTypePtr_t &base)
    : m_name(name),
      m_type_class(type_class),
      m_base(base),
      m_default_value(nullptr),
      m_id(0)
{
}

SymbolType::SymbolType(const std::string &name, 
    SymbolTypeClass type_class,
    const SymbolTypePtr_t &base,
    const std::shared_ptr<AstExpression> &default_value,
    const std::vector<SymbolMember_t> &members)
    : m_name(name),
      m_type_class(type_class),
      m_base(base),
      m_default_value(default_value),
      m_members(members),
      m_id(0)
{
}

SymbolType::SymbolType(const SymbolType &other)
    : m_name(other.m_name),
      m_type_class(other.m_type_class),
      m_base(other.m_base),
      m_default_value(other.m_default_value),
      m_members(other.m_members),
      m_id(other.m_id)
{
}

bool SymbolType::TypeEqual(const SymbolType &other) const
{
    if (m_type_class != other.m_type_class) {
        return false;
    }

    switch (m_type_class) {
        case TYPE_ALIAS: {
            auto sp = m_alias_info.m_aliasee.lock();
            return sp != nullptr && (*sp) == other;
        }
        case TYPE_FUNCTION:
            if (!m_function_info.m_return_type || !other.m_function_info.m_return_type) {
                return false;
            }

            if (!((*m_function_info.m_return_type) == (*other.m_function_info.m_return_type))) {
                return false;
            }

            for (const SymbolTypePtr_t &i : m_function_info.m_param_types) {
                if (!i) {
                    return false;
                }

                for (const SymbolTypePtr_t &j : other.m_function_info.m_param_types) {
                    if (!j || !((*i) == (*j))) {
                        return false;
                    }
                }
            }
            break;
        case TYPE_GENERIC:
            if (m_generic_info.m_num_parameters != other.m_generic_info.m_num_parameters) {
                return false;
            }
            break;
        case TYPE_GENERIC_INSTANCE:
            if (m_generic_instance_info.m_param_types.size() != other.m_generic_instance_info.m_param_types.size()) {
                return false;
            }

            // check each substituted parameter
            for (size_t i = 0; i < m_generic_instance_info.m_param_types.size(); i++) {
                ASSERT(m_generic_instance_info.m_param_types[i] != nullptr);
                ASSERT(other.m_generic_instance_info.m_param_types[i] != nullptr);

                if (!m_generic_instance_info.m_param_types[i]->TypeEqual(*other.m_generic_instance_info.m_param_types[i])) {
                    return false;
                }
            }

            break;
        default: break;
    }

    if (m_name != other.m_name) {
        return false;
    }

    if (m_members.size() != other.m_members.size()) {
        return false;
    }

    for (const SymbolMember_t &i : m_members) {
        ASSERT(std::get<1>(i) != nullptr);

        for (const SymbolMember_t &j : other.m_members) {
            ASSERT(std::get<1>(j) != nullptr);

            if (!(std::get<1>(i)->TypeEqual(*std::get<1>(j)))) {
                return false;
            }
        }
    }

    return true;
}

bool SymbolType::TypeCompatible(const SymbolType &right, bool strict_numbers) const
{
    /*if (right.TypeEqual(*SymbolType::Builtin::ANY)) {
        return true;
    }*/

    if (TypeEqual(right)) {
        return true;
    } else if (right.GetTypeClass() == TYPE_GENERIC_PARAMETER && 
                (!right.GetGenericParameterInfo().m_substitution.lock())) {
        // right is a generic paramter that has not yet been substituted
        return true;
    }

    switch (m_type_class) {
        case TYPE_ALIAS: {
            auto sp = m_alias_info.m_aliasee.lock();
            ASSERT(sp != nullptr);

            return sp->TypeCompatible(right, strict_numbers);
        }
        case TYPE_GENERIC: {

            if (right.m_type_class != TYPE_GENERIC) {
                if (auto other_base = right.m_base.lock()) {
                    return TypeCompatible(*other_base, strict_numbers);
                }
            } // equality would have already been checked

            return false;
        }
        case TYPE_GENERIC_INSTANCE: {
            auto base = m_base.lock();
            ASSERT(base != nullptr);

            if (right.m_type_class == TYPE_GENERIC_INSTANCE) {
                // check for compatibility between instances
                auto other_base = right.m_base.lock();
                ASSERT(other_base != nullptr);

                // check if bases are compatible
                if (base != other_base && !base->TypeCompatible(*other_base, strict_numbers)) {
                    return false;
                }

                // check all params
                if (m_generic_instance_info.m_param_types.size() != right.m_generic_instance_info.m_param_types.size()) {
                    return false;
                }

                // check each substituted parameter
                for (size_t i = 0; i < m_generic_instance_info.m_param_types.size(); i++) {
                    auto &param_type = m_generic_instance_info.m_param_types[i];
                    auto &other_param_type = right.m_generic_instance_info.m_param_types[i];

                    ASSERT(param_type != nullptr);
                    ASSERT(other_param_type != nullptr);

                    if (param_type != other_param_type && 
                        !param_type->TypeCompatible(*other_param_type, strict_numbers)) {
                        return false;
                    }
                }

                return true;
            } else {
                return false;
            }

            break;
        }
        case TYPE_BUILTIN:
            if (!TypeEqual(*SymbolType::Builtin::UNDEFINED) && !right.TypeEqual(*SymbolType::Builtin::UNDEFINED)) {
                if (TypeEqual(*SymbolType::Builtin::ANY)) {
                    return true;
                } else if (TypeEqual(*SymbolType::Builtin::NUMBER)) {
                    return right.TypeEqual(*SymbolType::Builtin::INT) ||
                        right.TypeEqual(*SymbolType::Builtin::FLOAT);
                } else if (!strict_numbers) {
                    if (TypeEqual(*SymbolType::Builtin::INT) || TypeEqual(*SymbolType::Builtin::FLOAT)) {
                        return right.TypeEqual(*SymbolType::Builtin::NUMBER) ||
                            right.TypeEqual(*SymbolType::Builtin::FLOAT) ||
                            right.TypeEqual(*SymbolType::Builtin::INT);
                    }
                }
            }

            return false;

        case TYPE_GENERIC_PARAMETER: {
            if (auto sp = m_generic_param_info.m_substitution.lock()) {
                return sp->TypeCompatible(right, strict_numbers);
            }
            return true;
        }
    }

    return true;
}

const SymbolTypePtr_t SymbolType::FindMember(const std::string &name) const
{
    for (const SymbolMember_t &member : m_members) {
        if (std::get<0>(member) == name) {
            return std::get<1>(member);
        }
    }

    return nullptr;
}

SymbolTypePtr_t SymbolType::Alias(const std::string &name, const AliasTypeInfo &info)
{
    SymbolTypePtr_t res;

    if (auto sp = info.m_aliasee.lock()) {
        auto base = sp->m_base.lock();
        ASSERT(base != nullptr);

        res.reset(new SymbolType(name, TYPE_ALIAS, base));
        res->m_alias_info = info;
    }

    return res;
}

SymbolTypePtr_t SymbolType::Primitive(const std::string &name, 
    const std::shared_ptr<AstExpression> &default_value)
{
    return SymbolTypePtr_t(new SymbolType(name, TYPE_BUILTIN, SymbolType::Builtin::OBJECT, default_value, {}));
}

SymbolTypePtr_t SymbolType::Primitive(const std::string &name,
    const std::shared_ptr<AstExpression> &default_value,
    const SymbolTypePtr_t &base)
{
    return SymbolTypePtr_t(new SymbolType(name, TYPE_BUILTIN, base, default_value, {}));
}

SymbolTypePtr_t SymbolType::Object(const std::string &name,
    const std::vector<SymbolMember_t> &members)
{
    SymbolTypePtr_t symbol_type(new SymbolType(name, TYPE_BUILTIN, SymbolType::Builtin::OBJECT, nullptr, members));
    symbol_type->SetDefaultValue(std::shared_ptr<AstObject>(
        new AstObject(symbol_type, SourceLocation::eof)));
    return symbol_type;
}

SymbolTypePtr_t SymbolType::Generic(const std::string &name, 
    const std::shared_ptr<AstExpression> &default_value,
    const std::vector<SymbolMember_t> &members, 
    const GenericTypeInfo &info)
{
    SymbolTypePtr_t res(new SymbolType(name, TYPE_GENERIC, nullptr, default_value, members));
    res->m_generic_info = info;
    return res;
}

SymbolTypePtr_t SymbolType::GenericInstance(const SymbolTypePtr_t &base,
    const GenericInstanceTypeInfo &info)
{
    ASSERT(base != nullptr);
    ASSERT(base->GetTypeClass() == TYPE_GENERIC);

    std::string name = base->GetName();
    if (!info.m_param_types.empty()) {
        name += "(";

        for (size_t i = 0; i < info.m_param_types.size(); i++) {
            name += info.m_param_types[i]->GetName();
            if (i != info.m_param_types.size() - 1) {
                name += ", ";
            }
        }

        name += ")";
    }

    std::vector<SymbolMember_t> members;
    members.reserve(base->GetMembers().size());

    for (const SymbolMember_t &member : base->GetMembers()) {
        bool substituted = false;

        if (std::get<1>(member)->GetTypeClass() == TYPE_GENERIC_PARAMETER) {
            // if members of the generic/template class are of the type T (generic parameter)
            // we need to make sure that the number of parameters supplied are equal.
            ASSERT(base->GetGenericInfo().m_params.size() == info.m_param_types.size());
            
            // find parameter and substitute it
            for (size_t i = 0; !substituted && i < base->GetGenericInfo().m_params.size(); i++) {
                auto &it = base->GetGenericInfo().m_params[i];
                if (it->GetName() == std::get<1>(member)->GetName()) {
                    std::shared_ptr<AstExpression> default_value;

                    if ((default_value = std::get<2>(member))) {
                        default_value = CloneAstNode(default_value);
                    }

                    members.push_back(SymbolMember_t(
                        std::get<0>(member), info.m_param_types[i], default_value));

                    substituted = true;
                }
            }

            if (!substituted) {
                // substitution error, set type to be undefined
                members.push_back(SymbolMember_t(
                    std::get<0>(member), SymbolType::Builtin::UNDEFINED, std::get<2>(member)));
            }
        } else {
            // push copy (clone assignment value)
            members.push_back(SymbolMember_t(
                std::get<0>(member), std::get<1>(member), CloneAstNode(std::get<2>(member))));
        }
    }

    // if the generic's default value is nullptr,
    // create a new default value for the instance of type AstObject
    // the reason we do this is so that a new 'Type' is generated for user-defined
    // generics, but built-in generics like Function and Array can play by
    // their own rules

    SymbolTypePtr_t res(new SymbolType(name, TYPE_GENERIC_INSTANCE, base,
        nullptr, members));

    auto default_value = base->GetDefaultValue();
    if (!default_value) {
        default_value.reset(new AstObject(res, SourceLocation::eof));
    }

    res->SetId(base->GetId());
    res->SetDefaultValue(default_value);
    res->m_generic_instance_info = info;
    return res;
}

SymbolTypePtr_t SymbolType::GenericParameter(const std::string &name, 
    const SymbolTypePtr_t &substitution)
{
    SymbolTypePtr_t res(new SymbolType(name, TYPE_GENERIC_PARAMETER, nullptr));
    res->m_generic_param_info.m_substitution = substitution;
    return res;
}

SymbolTypePtr_t SymbolType::TypePromotion(const SymbolTypePtr_t &lptr, const SymbolTypePtr_t &rptr, bool use_number)
{
    if (!lptr || !rptr) {
        return nullptr;
    }

    // compare pointer values
    if (lptr == rptr || lptr->TypeEqual(*rptr)) {
        return lptr;
    }

    if (lptr->TypeEqual(*SymbolType::Builtin::UNDEFINED) || rptr->TypeEqual(*SymbolType::Builtin::UNDEFINED)) {
        // (Undefined | Any) + (Undefined | Any) = Undefined
        return SymbolType::Builtin::UNDEFINED;
    } else if (lptr->TypeEqual(*SymbolType::Builtin::ANY)) {
        // Any + T = Any
        return SymbolType::Builtin::ANY;
    } else if (rptr->TypeEqual(*SymbolType::Builtin::ANY)) {
        // T + Any = Any
        return SymbolType::Builtin::ANY;//lptr;
    } else if (lptr->TypeEqual(*SymbolType::Builtin::NUMBER)) {
        return rptr->TypeEqual(*SymbolType::Builtin::INT) ||
               rptr->TypeEqual(*SymbolType::Builtin::FLOAT)
               ? SymbolType::Builtin::NUMBER
               : SymbolType::Builtin::UNDEFINED;
    } else if (lptr->TypeEqual(*SymbolType::Builtin::INT)) {
        return rptr->TypeEqual(*SymbolType::Builtin::NUMBER) ||
               rptr->TypeEqual(*SymbolType::Builtin::FLOAT)
               ? (use_number ? SymbolType::Builtin::NUMBER : rptr)
               : SymbolType::Builtin::UNDEFINED;
    } else if (lptr->TypeEqual(*SymbolType::Builtin::FLOAT)) {
        return rptr->TypeEqual(*SymbolType::Builtin::NUMBER) ||
               rptr->TypeEqual(*SymbolType::Builtin::INT)
               ? (use_number ? SymbolType::Builtin::NUMBER : lptr)
               : SymbolType::Builtin::UNDEFINED;
    } else if (rptr->TypeEqual(*SymbolType::Builtin::NUMBER)) {
        return lptr->TypeEqual(*SymbolType::Builtin::INT) ||
               lptr->TypeEqual(*SymbolType::Builtin::FLOAT)
               ? SymbolType::Builtin::NUMBER
               : SymbolType::Builtin::UNDEFINED;
    } else if (rptr->TypeEqual(*SymbolType::Builtin::INT)) {
        return lptr->TypeEqual(*SymbolType::Builtin::NUMBER) ||
               lptr->TypeEqual(*SymbolType::Builtin::FLOAT)
               ? (use_number ? SymbolType::Builtin::NUMBER : lptr)
               : SymbolType::Builtin::UNDEFINED;
    } else if (rptr->TypeEqual(*SymbolType::Builtin::FLOAT)) {
        return lptr->TypeEqual(*SymbolType::Builtin::NUMBER) ||
               lptr->TypeEqual(*SymbolType::Builtin::INT)
               ? (use_number ? SymbolType::Builtin::NUMBER : rptr)
               : SymbolType::Builtin::UNDEFINED;
    }

    return SymbolType::Builtin::UNDEFINED;
}