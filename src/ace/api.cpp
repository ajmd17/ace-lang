#include <ace/api.hpp>
#include <ace-c/parser.hpp>

#include <memory>
#include <vector>
#include <cassert>

namespace ace {

const std::string API::GLOBAL_MODULE_NAME = "__global__";

void API::BindNativeFunction(const NativeFunctionDefine &def,
    VM *vm, CompilationUnit *compilation_unit)
{
    assert(vm != nullptr);

    Module *mod = nullptr;

    for (std::unique_ptr<Module> &it : compilation_unit->m_modules) {
        if (it->GetName() == def.module_name) {
            mod = it.get();
            break;
        }
    }

    if (mod == nullptr) {
        // add this module to the compilation unit
        std::unique_ptr<Module> this_module(new Module(def.module_name, SourceLocation::eof));
        compilation_unit->m_modules.push_back(std::move(this_module));
        compilation_unit->m_module_index++;
        mod = compilation_unit->m_modules.back().get();
    }

    assert(mod != nullptr);

    // get global scope
    Scope &scope = mod->m_scopes.Top();

    // look up variable to make sure it doesn't already exist
    // only this scope matters, variables with the same name outside
    // of this scope are fine
    Identifier *ident = mod->LookUpIdentifier(def.function_name, true);
    assert(ident == nullptr &&
        "cannot create multiple objects with the same name");
    // add identifier
    ident = scope.GetIdentifierTable().AddIdentifier(def.function_name);
    assert(ident != nullptr);

    // create value
    std::vector<std::shared_ptr<AstParameter>> parameters; // TODO
    std::shared_ptr<AstBlock> block(new AstBlock(SourceLocation::eof));
    std::shared_ptr<AstFunctionExpression> value(
        new AstFunctionExpression(parameters, nullptr, block, SourceLocation::eof));

    value->SetReturnType(def.return_type);

    // set identifier info
    ident->SetFlags(FLAG_CONST);
    ident->SetObjectType(ObjectType::MakeFunctionType(def.return_type, def.param_types));
    ident->SetCurrentValue(value);
    ident->SetStackLocation(compilation_unit->GetInstructionStream().GetStackSize());
    compilation_unit->GetInstructionStream().IncStackSize();

    // finally, push to VM
    vm->PushNativeFunctionPtr(def.ptr);
}


APIInstance &APIInstance::Function(const std::string &module_name,
    const std::string &function_name,
    const ObjectType &return_type,
    const std::vector<ObjectType> &param_types,
    NativeFunctionPtr_t ptr)
{
    m_defs.push_back(API::NativeFunctionDefine(
        module_name, function_name, 
        return_type, param_types, ptr
    ));

    return *this;
}

void APIInstance::BindAll(VM *vm, CompilationUnit *compilation_unit)
{
    for (API::NativeFunctionDefine &def : m_defs) {
        API::BindNativeFunction(def, vm, compilation_unit);
    }
}

} // namespace ace