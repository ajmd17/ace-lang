#include <ace/API.hpp>

#include <ace-c/Parser.hpp>
#include <ace-c/Configuration.hpp>
#include <ace-c/ast/AstObject.hpp>

#include <common/my_assert.hpp>
#include <common/hasher.hpp>

#include <memory>
#include <vector>

namespace ace {

using namespace vm;

// modules that have been declared
static std::vector<std::shared_ptr<Module>> declared_modules;

static Module *GetModule(CompilationUnit *compilation_unit, const std::string &module_name)
{
    if (Module *mod = compilation_unit->LookupModule(module_name)) {
        return mod;
    }

    return nullptr;
}

static Identifier *CreateIdentifier(CompilationUnit *compilation_unit,
    Module *mod, const std::string &name)
{
    ASSERT(compilation_unit != nullptr);
    ASSERT(mod != nullptr);

    // get global scope
    Scope &scope = mod->m_scopes.Top();

    // look up variable to make sure it doesn't already exist
    // only this scope matters, variables with the same name outside
    // of this scope are fine
    Identifier *ident = mod->LookUpIdentifier(name, true);
    ASSERT_MSG(ident == nullptr, "Cannot create multiple objects with the same name");

    // add identifier
    ident = scope.GetIdentifierTable().AddIdentifier(name);

    return ident;
}

static std::string MangleName(const std::string &type_name, const std::string &name)
{
    return "__" + type_name + "_" + name;
}

API::TypeDefine &API::TypeDefine::Member(const std::string &member_name,
    const SymbolTypePtr_t &member_type,
    NativeInitializerPtr_t ptr)
{
    m_members.push_back(API::NativeVariableDefine(
        member_name, member_type, ptr
    ));
    return *this;
}

API::TypeDefine &API::TypeDefine::Method(const std::string &method_name,
    const SymbolTypePtr_t &return_type,
    const std::vector<std::pair<std::string, SymbolTypePtr_t>> &param_types,
    NativeFunctionPtr_t ptr)
{
    m_methods.push_back(API::NativeFunctionDefine(
        method_name, return_type, param_types, ptr
    ));
    return *this;
}


API::TypeDefine &API::ModuleDefine::Type(const std::string &type_name)
{
    // check if already found
    for (auto &def : m_types) {
        if (def.m_name == type_name) {
            return def;
        }
    }

    // not found, add new
    TypeDefine def;
    def.m_name = type_name;
    m_types.push_back(def);

    // return last
    return m_types.back();
}

API::ModuleDefine &API::ModuleDefine::Variable(
    const std::string &variable_name,
    const SymbolTypePtr_t &variable_type,
    NativeInitializerPtr_t ptr)
{
    m_variable_defs.push_back(API::NativeVariableDefine(variable_name, variable_type, ptr));
    return *this;
}

API::ModuleDefine &API::ModuleDefine::Function(
    const std::string &function_name,
    const SymbolTypePtr_t &return_type,
    const std::vector<std::pair<std::string, SymbolTypePtr_t>> &param_types,
    NativeFunctionPtr_t ptr)
{
    m_function_defs.push_back(API::NativeFunctionDefine(
        function_name, return_type, param_types, ptr
    ));

    return *this;
}

void API::ModuleDefine::BindAll(VM *vm, CompilationUnit *compilation_unit)
{
    // bind module
    Module *mod = GetModule(compilation_unit, m_name);

    bool close_mod = false;

    // create new module
    if (!mod) {
        close_mod = true;

        std::shared_ptr<Module> new_mod(new Module(m_name, SourceLocation::eof));
        declared_modules.push_back(new_mod);

        mod = new_mod.get();

        // add this module to the compilation unit
        compilation_unit->m_module_tree.Open(mod);
        // set the link to the module in the tree
        mod->SetImportTreeLink(compilation_unit->m_module_tree.TopNode());
    }

    for (auto &def : m_variable_defs) {
        BindNativeVariable(def, mod, vm, compilation_unit);
    }

    for (auto &def : m_function_defs) {
        BindNativeFunction(def, mod, vm, compilation_unit);
    }

    if (close_mod) {
        // close this module
        compilation_unit->m_module_tree.Close();
    }
}

void API::ModuleDefine::BindNativeVariable(const NativeVariableDefine &def,
    Module *mod, VM *vm, CompilationUnit *compilation_unit)
{
    ASSERT(mod != nullptr && vm != nullptr && compilation_unit != nullptr);

    Identifier *ident = CreateIdentifier(compilation_unit, mod, def.name);

    ASSERT(ident != nullptr);

    // create the value (set it to the default value of the type)
    ident->SetSymbolType(def.type);
    ident->SetCurrentValue(def.type->GetDefaultValue());
    ident->SetStackLocation(compilation_unit->GetInstructionStream().GetStackSize());
    compilation_unit->GetInstructionStream().IncStackSize();

    ASSERT(vm->GetState().GetNumThreads() > 0);

    ExecutionThread *main_thread = vm->GetState().m_threads[0];

    // create the object that will be stored
    Value obj;
    obj.m_type = Value::HEAP_POINTER;
    obj.m_value.ptr = nullptr;

    // call the initializer
    def.initializer_ptr(&vm->GetState(), main_thread, &obj);

    // push the object to the main thread's stack
    main_thread->GetStack().Push(obj);
}

void API::ModuleDefine::BindNativeFunction(
    const NativeFunctionDefine &def,
    Module *mod, VM *vm, CompilationUnit *compilation_unit)
{
    ASSERT(mod != nullptr && vm != nullptr && compilation_unit != nullptr);


    Identifier *ident = CreateIdentifier(compilation_unit, mod, def.function_name);
    ASSERT(ident != nullptr);

    // create value
    std::vector<std::shared_ptr<AstParameter>> parameters; // TODO
    std::shared_ptr<AstBlock> block(new AstBlock(SourceLocation::eof));
    std::shared_ptr<AstFunctionExpression> value(new AstFunctionExpression(
        parameters, nullptr, block, false, false, SourceLocation::eof
    ));

    value->SetReturnType(def.return_type);

    std::vector<std::pair<std::string, SymbolTypePtr_t>> generic_param_types;
    generic_param_types.push_back({
        "@return", def.return_type
    });
    for (auto &it : def.param_types) {
        generic_param_types.push_back(it);
    }

    // set identifier info
    ident->SetFlags(FLAG_CONST);
    ident->SetSymbolType(SymbolType::GenericInstance(
        SymbolType::Builtin::FUNCTION, 
        GenericInstanceTypeInfo { generic_param_types }
    ));
    ident->SetCurrentValue(value);
    ident->SetStackLocation(compilation_unit->GetInstructionStream().GetStackSize());
    compilation_unit->GetInstructionStream().IncStackSize();

    // finally, push to VM
    vm->PushNativeFunctionPtr(def.ptr);
}

/*void API::ModuleDefine::BindType(const TypeDefine &def,
    Module *mod, VM *vm, CompilationUnit *compilation_unit)
{
    ASSERT(mod != nullptr && vm != nullptr && compilation_unit != nullptr);

    ObjectType object_type(def.m_name, nullptr);

    // look up to make sure type doesn't exist
    ASSERT_MSG(!mod->LookUpUserType(def.m_name, object_type), "Type already defined");
    ASSERT_MSG(!ObjectType::GetBuiltinType(def.m_name), "Type already defined as built-in type");

    unsigned int num_members = (unsigned int)(def.m_members.size() + def.m_methods.size());

    // generate hashes for member names
    std::vector<uint32_t> hashes;
    hashes.reserve(num_members);

    // the start on the stack of where the methods are.
    const int stack_start = compilation_unit->GetInstructionStream().GetStackSize();

    for (size_t i = 0; i < def.m_methods.size(); i++) {
        const auto &method = def.m_methods[i];
        auto method_copy = method;
        method_copy.function_name = MangleName(def.m_name, method.function_name);

        BindNativeFunction(method_copy, mod, vm, compilation_unit);
    }

    // open the scope for data members
    mod->m_scopes.Open(Scope());

    for (const auto &mem : def.m_members) {
        Scope &scope = mod->m_scopes.Top();

        ASSERT_MSG(!object_type.HasDataMember(mem.name), "Type has duplicate data member");

        DataMember dm(mem.name, mem.type);
        object_type.AddDataMember(dm);

        // generate hash from member name
        hashes.push_back(hash_fnv_1(mem.name.c_str()));
    }

    // create methods
    for (size_t i = 0; i < def.m_methods.size(); i++) {
        Scope &scope = mod->m_scopes.Top();

        const auto &method = def.m_methods[i];

        std::string mangled_name = MangleName(def.m_name, method.function_name);
        Identifier *mangled_ident = mod->LookUpIdentifier(mangled_name, false);
        ASSERT(mangled_ident != nullptr);

        // create identifier
        Identifier *ident = mod->LookUpIdentifier(method.function_name, true);
        ASSERT_MSG(ident == nullptr, "Cannot create multiple methods with the same name");

        // add identifier for method
        ident = scope.GetIdentifierTable().AddIdentifier(method.function_name);
        ASSERT(ident != nullptr);

        ObjectType member_type = ObjectType::MakeFunctionType(method.return_type, method.param_types);

        auto value = std::shared_ptr<AstVariable>(new AstVariable(mangled_name, SourceLocation::eof));
        value->GetProperties().SetIdentifier(mangled_ident);

        member_type.SetDefaultValue(value);

        // set identifier info
        ident->SetObjectType(member_type);

        DataMember dm(method.function_name, member_type);
        // add data member
        object_type.AddDataMember(dm);

        // generate hash from method name
        hashes.push_back(hash_fnv_1(method.function_name.c_str()));
    }

    // close the scope for data members
    mod->m_scopes.Close();

    ASSERT((int)num_members < ace::compiler::Config::MAX_DATA_MEMBERS);

    // mangle the type name
    std::string type_name = mod->GetName() + "." + m_name;
    size_t len = type_name.length();

    // create static object
    StaticTypeInfo st;
    st.m_size = num_members;
    st.m_hashes = hashes;
    st.m_name = new char[len + 1];
    st.m_name[len] = '\0';
    std::strcpy(st.m_name, type_name.c_str());

    StaticObject so(st);
    int found_id = compilation_unit->GetInstructionStream().FindStaticObject(so);
    if (found_id == -1) {
        so.m_id = compilation_unit->GetInstructionStream().NewStaticId();
        compilation_unit->GetInstructionStream().AddStaticObject(so);
        object_type.SetStaticId(so.m_id);
    } else {
        object_type.SetStaticId(found_id);
    }

    delete[] st.m_name;

    object_type.SetDefaultValue(std::shared_ptr<AstObject>(new AstObject(object_type, SourceLocation::eof)));
    mod->AddUserType(object_type);
}*/

API::ModuleDefine &APIInstance::Module(const std::string &name)
{
    // check if created already first
    for (auto &def : m_module_defs) {
        if (def.m_name == name) {
            return def;
        }
    }

    // not found, add module
    API::ModuleDefine def;
    def.m_name = name;

    // return the new instance
    m_module_defs.push_back(def);
    return m_module_defs.back();
}

void APIInstance::BindAll(VM *vm, CompilationUnit *compilation_unit)
{
    for (auto &module_def : m_module_defs) {
        module_def.BindAll(vm, compilation_unit);
    }
}

} // namespace ace
