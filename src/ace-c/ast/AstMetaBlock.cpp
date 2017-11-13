#include <ace-c/ast/AstMetaBlock.hpp>
#include <ace-c/AstVisitor.hpp>
#include <ace-c/Lexer.hpp>
#include <ace-c/Parser.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/CompilationUnit.hpp>
#include <ace-c/Configuration.hpp>

#include <ace-c/type-system/BuiltinTypes.hpp>

#include <ace-c/meta-scripting/API.hpp>

#include <ace-c/emit/BytecodeChunk.hpp>
#include <ace-c/emit/BytecodeUtil.hpp>
#include <ace-c/emit/aex-builder/AEXGenerator.hpp>

#include <ace-vm/VM.hpp>
#include <ace-vm/BytecodeStream.hpp>
#include <ace-vm/Array.hpp>
#include <ace-vm/Object.hpp>
#include <ace-vm/TypeInfo.hpp>

#include <common/hasher.hpp>

#include <functional>
#include <cstdint>
#include <iostream>

using namespace ace;

AstMetaBlock::AstMetaBlock(
    const std::shared_ptr<AstBlock> &block,
    const SourceLocation &location)
    : AstStatement(location),
      m_block(block)
{
}

void AstMetaBlock::Visit(AstVisitor *visitor, Module *mod)
{
    ASSERT(m_block != nullptr);

    struct {
        AstVisitor *m_visitor;
        Module *m_mod;
    } meta_context;

    meta_context.m_visitor = visitor;
    meta_context.m_mod = mod;

    AstIterator ast_iterator;
    ast_iterator.Push(m_block);

    vm::VM vm;
    CompilationUnit compilation_unit;

    APIInstance meta_api;

    meta_api.Module(compiler::Config::global_module_name)
        .Variable("__meta_context", BuiltinTypes::ANY, (UserData_t)&meta_context)
        .Variable("compiler", BuiltinTypes::ANY, [](vm::VMState *state, vm::ExecutionThread *thread, vm::Value *out) {
            ASSERT(state != nullptr);
            ASSERT(out != nullptr);

            NativeFunctionPtr_t compiler_define_func = [](ace::sdk::Params params) {
                std::cout << "HELLO\n";
            };

            vm::Member members[1]; // TODO make a builder class or something to do this
            std::strcpy(members[0].name, "define");
            members[0].hash = hash_fnv_1("define");
            members[0].value.m_type = vm::Value::NATIVE_FUNCTION;
            members[0].value.m_value.native_func = compiler_define_func;

            // create prototype object.
            vm::HeapValue *proto = state->HeapAlloc(thread);
            ASSERT(proto != nullptr);
            proto->Assign(vm::Object(&members[0], sizeof(members) / sizeof(members[0])));

            // create Object instance
            vm::Object ins(proto);
            // TODO

            vm::HeapValue *object = state->HeapAlloc(thread);
            ASSERT(object != nullptr);
            object->Assign(ins);
            
            // assign the out value to this
            out->m_type = vm::Value::ValueType::HEAP_POINTER;
            out->m_value.ptr = object;
        });
    
    meta_api.BindAll(&vm, &compilation_unit);

    SemanticAnalyzer meta_analyzer(&ast_iterator, &compilation_unit);
    meta_analyzer.Analyze();

    if (!compilation_unit.GetErrorList().HasFatalErrors()) {
        // build in-place
        Compiler meta_compiler(&ast_iterator, &compilation_unit);
        ast_iterator.ResetPosition();

        std::unique_ptr<BytecodeChunk> result = meta_compiler.Compile();

        BuildParams build_params;
        build_params.block_offset = 0;
        build_params.local_offset = 0;

        AEXGenerator gen(build_params);
        gen.Visit(result.get());

        std::vector<std::uint8_t> bytes = gen.GetInternalByteStream().Bake();

        vm::BytecodeStream bs(reinterpret_cast<char*>(&bytes[0]), bytes.size());

        vm.Execute(&bs);
    }

    // concatenate errors to this compilation unit
    visitor->GetCompilationUnit()->GetErrorList().Concatenate(compilation_unit.GetErrorList());
}

std::unique_ptr<Buildable> AstMetaBlock::Build(AstVisitor *visitor, Module *mod)
{
    return nullptr;
}

void AstMetaBlock::Optimize(AstVisitor *visitor, Module *mod)
{
}

Pointer<AstStatement> AstMetaBlock::Clone() const
{
    return CloneImpl();
}
