#include <ace-c/meta-scripting/Meta.hpp>
#include <ace-sdk/ace-sdk.hpp>

#include <ace-vm/Value.hpp>

using namespace ace;

void MetaDefine(sdk::Params params)
{
    ACE_CHECK_ARGS(==, 4);

    // arguments should be:
    //   __meta_context object (UserData)
    //   identifier (String)
    //   value (Any)

    vm::Value *meta_context_ptr = params.args[0];
    ASSERT(meta_context_ptr != nullptr);
    ASSERT(meta_context_ptr->m_type == vm::Value::USER_DATA);
}

void Meta::BuildMetaLibrary(vm::VM &vm,
    CompilationUnit &compilation_unit,
    APIInstance &api)
{
}