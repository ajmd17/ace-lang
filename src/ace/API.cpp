#include <ace/API.hpp>

#include <ace-c/Parser.hpp>
#include <ace-c/Configuration.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

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

static Identifier *CreateIdentifier(
    CompilationUnit *compilation_unit,
    Module *mod,
    const std::string &name)
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

/*API::TypeDefine &API::TypeDefine::Member(const std::string &member_name,
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
}*/

API::ModuleDefine &API::ModuleDefine::Type(const SymbolTypePtr_t &type)
{
    m_type_defs.push_back(TypeDefine {
        type
    });

    // return last
    return *this;
}

API::ModuleDefine &API::ModuleDefine::Variable(
    const std::string &variable_name,
    const SymbolTypePtr_t &variable_type,
    NativeInitializerPtr_t ptr)
{
    m_variable_defs.push_back(API::NativeVariableDefine(
        variable_name,
        variable_type,
        ptr
    ));

    return *this;
}

API::ModuleDefine &API::ModuleDefine::Function(
    const std::string &function_name,
    const SymbolTypePtr_t &return_type,
    const std::vector<GenericInstanceTypeInfo::Arg> &param_types,
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
    if (mod == nullptr) {
        close_mod = true;

        std::shared_ptr<Module> new_mod(new Module(m_name, SourceLocation::eof));
        declared_modules.push_back(new_mod);

        mod = new_mod.get();

        // add this module to the compilation unit
        compilation_unit->m_module_tree.Open(mod);
        // set the link to the module in the tree
        mod->SetImportTreeLink(compilation_unit->m_module_tree.TopNode());

        // open a new scope for this module
        //mod->m_scopes.Open(Scope());
    }

    for (auto &def : m_variable_defs) {
        BindNativeVariable(def, mod, vm, compilation_unit);
    }

    for (auto &def : m_function_defs) {
        BindNativeFunction(def, mod, vm, compilation_unit);
    }

    for (auto &def : m_type_defs) {
        BindType(def, mod, vm, compilation_unit);
    }

    if (close_mod) {
        ASSERT(mod != nullptr);
        // close scope for module
       // mod->m_scopes.Close();
        // close this module
        compilation_unit->m_module_tree.Close();
    }
}

void API::ModuleDefine::BindNativeVariable(
    const NativeVariableDefine &def,
    Module *mod,
    VM *vm,
    CompilationUnit *compilation_unit)
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
    Module *mod,
    VM *vm,
    CompilationUnit *compilation_unit)
{
    ASSERT(mod != nullptr && vm != nullptr && compilation_unit != nullptr);

    Identifier *ident = CreateIdentifier(compilation_unit, mod, def.function_name);
    ASSERT(ident != nullptr);

    // create value
    std::vector<std::shared_ptr<AstParameter>> parameters; // TODO
    std::shared_ptr<AstBlock> block(new AstBlock(SourceLocation::eof));
    std::shared_ptr<AstFunctionExpression> value(new AstFunctionExpression(
        parameters, nullptr, block, false, false, false, SourceLocation::eof
    ));

    value->SetReturnType(def.return_type);

    std::vector<GenericInstanceTypeInfo::Arg> generic_param_types;

    generic_param_types.push_back({
        "@return", def.return_type
    });

    for (auto &it : def.param_types) {
        generic_param_types.push_back(it);
    }

    // set identifier info
    ident->SetFlags(FLAG_CONST);
    ident->SetSymbolType(SymbolType::GenericInstance(
        BuiltinTypes::FUNCTION, 
        GenericInstanceTypeInfo { generic_param_types }
    ));
    ident->SetCurrentValue(value);
    ident->SetStackLocation(compilation_unit->GetInstructionStream().GetStackSize());
    compilation_unit->GetInstructionStream().IncStackSize();

    // finally, push to VM
    vm->PushNativeFunctionPtr(def.ptr);
}

void API::ModuleDefine::BindType(TypeDefine def,
    Module *mod,
    VM *vm,
    CompilationUnit *compilation_unit)
{
    ASSERT(mod != nullptr && vm != nullptr && compilation_unit != nullptr);
    ASSERT(def.m_type != nullptr);

    const std::string type_name = def.m_type->GetName();

    if (mod->LookupSymbolType(type_name)) {
        // error; redeclaration of type in module
        compilation_unit->GetErrorList().AddError(
            CompilerError(
                LEVEL_ERROR,
                Msg_redefined_type,
                SourceLocation::eof,
                type_name
            )
        );
    } else {
        compilation_unit->RegisterType(def.m_type);
        // add the type to the identifier table, so it's usable
        Scope &top_scope = mod->m_scopes.Top();
        top_scope.GetIdentifierTable().AddSymbolType(def.m_type);
    }
}

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
