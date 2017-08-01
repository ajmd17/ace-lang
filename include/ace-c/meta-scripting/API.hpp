#ifndef API_HPP
#define API_HPP

#include <ace-c/Configuration.hpp>
#include <ace-c/CompilationUnit.hpp>
#include <ace-c/type-system/SymbolType.hpp>

#include <ace-vm/Value.hpp>
#include <ace-vm/VM.hpp>

#include <ace-sdk/ace-sdk.hpp>

#include <string>
#include <vector>

namespace ace {

class API {
public:
    struct NativeVariableDefine {
        std::string name;
        SymbolTypePtr_t type;

        enum { INITIALIZER, USER_DATA } value_type;
        union NativeVariableData {
            NativeInitializerPtr_t initializer_ptr;
            UserData_t user_data;

            NativeVariableData(NativeInitializerPtr_t initializer_ptr)
                : initializer_ptr(initializer_ptr)
            {
            }

            NativeVariableData(UserData_t user_data)
                : user_data(user_data)
            {
            }
        } value;

        NativeVariableDefine(
            const std::string &name,
            const SymbolTypePtr_t &type,
            NativeInitializerPtr_t initializer_ptr)
            : name(name),
              type(type),
              value_type(INITIALIZER),
              value(initializer_ptr)
        {
        }

        NativeVariableDefine(
            const std::string &name,
            const SymbolTypePtr_t &type,
            UserData_t user_data)
            : name(name),
              type(type),
              value_type(USER_DATA),
              value(user_data)
        {
        }

        NativeVariableDefine(const NativeVariableDefine &other)
            : name(other.name),
              type(other.type),
              value_type(other.value_type),
              value(other.value)
        {
        }
    };

    struct NativeFunctionDefine {
        std::string function_name;
        SymbolTypePtr_t return_type;
        std::vector<GenericInstanceTypeInfo::Arg> param_types;
        NativeFunctionPtr_t ptr;

        NativeFunctionDefine(
            const std::string &function_name,
            const SymbolTypePtr_t &return_type,
            const std::vector<GenericInstanceTypeInfo::Arg> &param_types,
            NativeFunctionPtr_t ptr)
            : function_name(function_name),
              return_type(return_type),
              param_types(param_types),
              ptr(ptr)
        {
        }

        NativeFunctionDefine(const NativeFunctionDefine &other)
            : function_name(other.function_name),
              return_type(other.return_type),
              param_types(other.param_types),
              ptr(other.ptr)
        {
        }
    };

    struct TypeDefine {
    public:
        /*std::string m_name;
        std::vector<NativeFunctionDefine> m_methods;
        std::vector<NativeVariableDefine> m_members;

        TypeDefine &Member(const std::string &member_name,
            const SymbolTypePtr_t &member_type,
            vm::NativeInitializerPtr_t ptr);

        TypeDefine &Method(const std::string &method_name,
            const SymbolTypePtr_t &return_type,
            const std::vector<std::pair<std::string, SymbolTypePtr_t>> &param_types,
            vm::NativeFunctionPtr_t ptr);*/

        SymbolTypePtr_t m_type;
    };

    struct ModuleDefine {
    public:
        std::string m_name;
        std::vector<TypeDefine> m_type_defs;
        std::vector<NativeFunctionDefine> m_function_defs;
        std::vector<NativeVariableDefine> m_variable_defs;
        std::shared_ptr<Module> m_mod;

        ModuleDefine &Type(const SymbolTypePtr_t &type);

        ModuleDefine &Variable(const std::string &variable_name,
            const SymbolTypePtr_t &variable_type,
            NativeInitializerPtr_t ptr);

        ModuleDefine &Variable(const std::string &variable_name,
            const SymbolTypePtr_t &variable_type,
            UserData_t ptr);

        ModuleDefine &Function(const std::string &function_name,
            const SymbolTypePtr_t &return_type,
            const std::vector<GenericInstanceTypeInfo::Arg> &param_types,
            NativeFunctionPtr_t ptr);

        void BindAll(vm::VM *vm, CompilationUnit *compilation_unit);

    private:
        void BindNativeVariable(const NativeVariableDefine &def,
            Module *mod, vm::VM *vm, CompilationUnit *compilation_unit);

        void BindNativeFunction(const NativeFunctionDefine &def,
            Module *mod, vm::VM *vm, CompilationUnit *compilation_unit);

        void BindType(TypeDefine def,
            Module *mod, vm::VM *vm, CompilationUnit *compilation_unit);
    };
};

class APIInstance {
public:
    APIInstance() {}
    APIInstance(const APIInstance &other) = delete;
    ~APIInstance() {}

    API::ModuleDefine &Module(const std::string &name);

    void BindAll(vm::VM *vm, CompilationUnit *compilation_unit);

private:
    std::vector<API::ModuleDefine> m_module_defs;
};

} // namespace ace

#endif