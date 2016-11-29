#include <ace-c/ace-c.hpp>
#include <ace-c/configuration.hpp>
#include <ace-c/semantic_analyzer.hpp>
#include <ace-c/optimizer.hpp>
#include <ace-c/compilation_unit.hpp>
#include <ace-c/lexer.hpp>
#include <ace-c/parser.hpp>
#include <ace-c/compiler.hpp>

#include <ace-vm/ace-vm.hpp>

#include <common/utf8.hpp>
#include <common/str_util.hpp>

#include <string>
#include <sstream>
#include <cassert>
#include <cstdio>
#include <cstdlib>

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

void AddNativeFunction(const NativeFunctionDefine &def,
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

void Runtime_gc(VMState *state, StackValue **args, int nargs)
{
    /*std::stringstream ss;
    // dump heap to stringstream
    ss << "Before:\n" << state->m_heap << "\n\n";
    utf::cout << utf::Utf8String(ss.str().c_str());
    // clear stringstream
    ss.str("");*/

    if (nargs != 0) {
        state->ThrowException(Exception::InvalidArgsException(0, nargs));
        return;
    }
    
    // run the gc
    state->m_exec_thread.m_stack.MarkAll();
    state->m_heap.Sweep();

  /*  // dump heap to stringstream, after GC
    ss << "After:\n" << state->m_heap << "\n\n";
    utf::cout << utf::Utf8String(ss.str().c_str());*/
}

void Global_to_string(VMState *state, StackValue **args, int nargs)
{
    if (nargs != 1) {
        state->ThrowException(Exception::InvalidArgsException(1, nargs));
        return;
    }

    // create heap value for string
    HeapValue *ptr = state->HeapAlloc();
    StackValue &res = state->GetExecutionThread().GetRegisters()[0];
    if (ptr != nullptr) {
        ptr->Assign(args[0]->ToString());
        // assign register value to the allocated object
        res.m_type = StackValue::HEAP_POINTER;
        res.m_value.ptr = ptr;
    }
}

int main(int argc, char *argv[])
{
    utf::init();

    if (argc == 1) {
        // disable static objects, so we can append the bytecode files.
        ace::compiler::Config::use_static_objects = false;
        // do not cull unused objects
        ace::compiler::Config::cull_unused_objects = false;

        // the current compilation unit for REPL
        CompilationUnit compilation_unit;
        AstIterator ast_iterator;

        // REPL VM
        VM *vm = new VM;

        // bind native function library
        std::vector<NativeFunctionDefine> native_functions = {
            NativeFunctionDefine("Runtime", "gc", ObjectType::type_builtin_void, {}, Runtime_gc),
            NativeFunctionDefine("__global__", "to_string", ObjectType::type_builtin_string, {}, Global_to_string)
        };

        for (auto &def : native_functions) {
            AddNativeFunction(def, vm, &compilation_unit);
        }

        utf::Utf8String out_filename = "tmp.aex";
        std::ofstream out_file(out_filename.GetData(),
            std::ios::out | std::ios::binary | std::ios::trunc);
        out_file.close();

        int block_counter = 0;
        std::string code = "// Ace REPL\nmodule repl;\n";
        utf::cout << code;

        { // compile template code
            // send code to be compiled
            SourceFile source_file("<stdin>", code.length());
            std::memcpy(source_file.GetBuffer(), code.c_str(), code.length());
            SourceStream source_stream(&source_file);

            TokenStream token_stream;
            Lexer lex(source_stream, &token_stream, &compilation_unit);
            lex.Analyze();

            Parser parser(&ast_iterator, &token_stream, &compilation_unit);
            parser.Parse(true);

            SemanticAnalyzer semantic_analyzer(&ast_iterator, &compilation_unit);
            semantic_analyzer.Analyze(true);
        }

        std::string line = "";
        std::string tmp_line;

        utf::cout << "> ";
        while (std::getline(utf::cin, tmp_line)) {
            for (char ch : tmp_line) {
                if (ch == '{') {
                    block_counter++;
                } else if (ch == '}') {
                    block_counter--;
                }
            }

            line += tmp_line + '\n';

            if (block_counter <= 0) {
                // so we can rewind upon errors
                int old_pos = ast_iterator.GetPosition();

                // send code to be compiled
                SourceFile source_file("<stdin>", line.length());
                std::memcpy(source_file.GetBuffer(), line.c_str(), line.length());
                SourceStream source_stream(&source_file);

                TokenStream token_stream;
                Lexer lex(source_stream, &token_stream, &compilation_unit);
                lex.Analyze();

                Parser parser(&ast_iterator, &token_stream, &compilation_unit);
                parser.Parse(false);

                // in REPL mode only analyze if parsing and lexing went okay.
                SemanticAnalyzer semantic_analyzer(&ast_iterator, &compilation_unit);
                semantic_analyzer.Analyze(false);

                compilation_unit.GetErrorList().SortErrors();
                for (CompilerError &error : compilation_unit.GetErrorList().m_errors) {
                    utf::cout
                        << utf::Utf8String(error.GetLocation().GetFileName().c_str()) << " "
                        << "[" << (error.GetLocation().GetLine() + 1)
                        << ", " << (error.GetLocation().GetColumn() + 1)
                        << "]: " << utf::Utf8String(error.GetText().c_str()) << "\n";
                }

                if (!compilation_unit.GetErrorList().HasFatalErrors()) {
                    // only optimize if there were no errors
                    // before this point
                    ast_iterator.SetPosition(old_pos);
                    Optimizer optimizer(&ast_iterator, &compilation_unit);
                    optimizer.Optimize(false);

                    // compile into bytecode instructions
                    ast_iterator.SetPosition(old_pos);
                    Compiler compiler(&ast_iterator, &compilation_unit);
                    compiler.Compile(false);

                    // emit bytecode instructions to file
                    std::ofstream out_file(out_filename.GetData(),
                        std::ios::out | std::ios::binary | std::ios::app | std::ios::ate);

                    int file_pos = (int)out_file.tellp();

                    if (!out_file.is_open()) {
                        utf::cout << "Could not open file for writing: " << out_filename << "\n";
                        for (int i = old_pos; i < ast_iterator.GetPosition(); i++) {
                            ast_iterator.Pop();
                        }
                        ast_iterator.SetPosition(old_pos);
                    } else {
                        out_file << compilation_unit.GetInstructionStream();
                        compilation_unit.GetInstructionStream().ClearInstructions();
                        out_file.close();
                        ace_vm::RunBytecodeFile(vm, out_filename, file_pos);
                    }
                } else {
                    for (int i = old_pos; i < ast_iterator.GetPosition(); i++) {
                        ast_iterator.Pop();
                    }
                    ast_iterator.SetPosition(old_pos);
                    compilation_unit.GetErrorList().ClearErrors();
                }

                line.clear();

                utf::cout << "> ";
            } else {
                for (int i = 0; i < block_counter; i++) {
                    utf::cout << "... ";
                }
            }
        }

        delete vm;

    } else if (argc == 2) {
        utf::Utf8String src_filename = argv[1];
        utf::Utf8String out_filename = (str_util::strip_extension(argv[1]) + ".aex").c_str();

        // compile source file
        ace_compiler::BuildSourceFile(src_filename, out_filename);
        // execute the bytecode file
        ace_vm::RunBytecodeFile(out_filename);
    } else {
        utf::cout << "Invalid arguments";
    }

    return 0;
}
