#ifndef API_HPP
#define API_HPP

#include <ace-c/compilation_unit.hpp>
#include <ace-vm/ace-vm.hpp>

#include <string>
#include <vector>

namespace ace {

class API {
public:
    static const std::string GLOBAL_MODULE_NAME;

public:
    struct NativeFunctionDefine {
        std::string module_name;
        std::string function_name;
        ObjectType return_type;
        std::vector<ObjectType> param_types;
        NativeFunctionPtr_t ptr;

        NativeFunctionDefine(
            const std::string &module_name,
            const std::string &function_name,
            const ObjectType &return_type,
            const std::vector<ObjectType> &param_types,
            NativeFunctionPtr_t ptr)
            : module_name(module_name),
              function_name(function_name),
              return_type(return_type),
              param_types(param_types),
              ptr(ptr)
        {
        }

        NativeFunctionDefine(const NativeFunctionDefine &other)
            : module_name(other.module_name),
              function_name(other.function_name),
              return_type(other.return_type),
              param_types(other.param_types),
              ptr(other.ptr)
        {
        }
    };

    static void BindNativeFunction(const NativeFunctionDefine &def,
        VM *vm, CompilationUnit *compilation_unit);
};

class APIInstance {
public:
    APIInstance() {}
    APIInstance(const APIInstance &other) = delete;
    ~APIInstance() {}

    APIInstance &Function(const std::string &module_name,
        const std::string &function_name,
        const ObjectType &return_type,
        const std::vector<ObjectType> &param_types,
        NativeFunctionPtr_t ptr);

    void BindAll(VM *vm, CompilationUnit *compilation_unit);

private:
    std::vector<API::NativeFunctionDefine> m_defs;
};

} // namespace ace

#endif