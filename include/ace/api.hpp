#ifndef API_HPP
#define API_HPP

#include <ace-c/configuration.hpp>
#include <ace-c/compilation_unit.hpp>

#include <ace-vm/stack_value.hpp>
#include <ace-vm/vm.hpp>

#include <string>
#include <vector>

namespace ace {

class API {
public:
    struct NativeVariableDefine {
        std::string name;
        ObjectType type;
        vm::NativeInitializerPtr_t initializer_ptr;

        NativeVariableDefine(
            const std::string &name,
            const ObjectType &type,
            vm::NativeInitializerPtr_t initializer_ptr)
            : name(name),
              type(type),
              initializer_ptr(initializer_ptr)
        {
        }

        NativeVariableDefine(const NativeVariableDefine &other)
            : name(other.name),
              type(other.type),
              initializer_ptr(other.initializer_ptr)
        {
        }
    };

    struct NativeFunctionDefine {
        std::string function_name;
        ObjectType return_type;
        std::vector<ObjectType> param_types;
        vm::NativeFunctionPtr_t ptr;

        NativeFunctionDefine(
            const std::string &function_name,
            const ObjectType &return_type,
            const std::vector<ObjectType> &param_types,
            vm::NativeFunctionPtr_t ptr)
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
        std::string m_name;
        std::vector<NativeFunctionDefine> m_methods;
        std::vector<NativeVariableDefine> m_members;

        TypeDefine &Member(const std::string &member_name,
            const ObjectType &member_type,
            vm::NativeInitializerPtr_t ptr);

        TypeDefine &Method(const std::string &method_name,
            const ObjectType &return_type,
            const std::vector<ObjectType> &param_types,
            vm::NativeFunctionPtr_t ptr);
    };

    struct ModuleDefine {
    public:
        std::string m_name;
        std::vector<TypeDefine> m_types;
        std::vector<NativeFunctionDefine> m_function_defs;
        std::vector<NativeVariableDefine> m_variable_defs;

        TypeDefine &Type(const std::string &type_name);

        ModuleDefine &Variable(const std::string &variable_name,
            const ObjectType &variable_type,
            vm::NativeInitializerPtr_t ptr);

        ModuleDefine &Function(const std::string &function_name,
            const ObjectType &return_type,
            const std::vector<ObjectType> &param_types,
            vm::NativeFunctionPtr_t ptr);

        void BindAll(vm::VM *vm, CompilationUnit *compilation_unit);

    private:
        void BindNativeVariable(const NativeVariableDefine &def,
            Module *mod, vm::VM *vm, CompilationUnit *compilation_unit);

        void BindNativeFunction(const NativeFunctionDefine &def,
            Module *mod, vm::VM *vm, CompilationUnit *compilation_unit);

        void BindType(const TypeDefine &def,
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