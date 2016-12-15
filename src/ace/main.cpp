#include <ace/api.hpp>
#include <ace/runtime.hpp>

#include <ace-c/ace-c.hpp>
#include <ace-c/configuration.hpp>
#include <ace-c/semantic_analyzer.hpp>
#include <ace-c/optimizer.hpp>
#include <ace-c/lexer.hpp>
#include <ace-c/parser.hpp>
#include <ace-c/compiler.hpp>

#include <ace-vm/object.hpp>
#include <ace-vm/array.hpp>

#include <common/cli_args.hpp>
#include <common/str_util.hpp>
#include <common/instructions.hpp>

#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <mutex>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <ace-vm/stack_value.hpp>

using namespace ace;

std::mutex mtx;
// all cpp threads
std::vector<std::thread> threads;

void Runtime_gc(ace::sdk::Params params)
{
    size_t heap_size_before = params.state->GetHeap().Size();

    if (params.nargs != 0) {
        params.state->ThrowException(params.thread, Exception::InvalidArgsException(0, params.nargs));
        return;
    }

    params.state->GC();

    size_t heap_size_after = params.state->GetHeap().Size();
    utf::cout << (heap_size_before - heap_size_after) << " object(s) collected.\n";
}

void Runtime_load_library(ace::sdk::Params params)
{
    if (params.nargs != 1) {
        params.state->ThrowException(params.thread, Exception::InvalidArgsException(1, params.nargs));
        return;
    }

    StackValue *target_ptr = params.args[0];
    ASSERT(target_ptr != nullptr);

    Exception e = Exception(utf::Utf8String("load_library() expects a String as the first argument"));

    if (target_ptr->GetType() == StackValue::ValueType::HEAP_POINTER) {
        utf::Utf8String *strptr = nullptr;

        if (target_ptr->GetValue().ptr == nullptr) {
            params.state->ThrowException(params.thread, Exception::NullReferenceException());
        } else if ((strptr = target_ptr->GetValue().ptr->GetPointer<utf::Utf8String>()) != nullptr) {
            // load library from string
            Library lib = Runtime::LoadLibrary(strptr->GetData());

            if (!lib.GetHandle()) {
                // could not load library
                params.state->ThrowException(params.thread, Exception::LibraryLoadException(strptr->GetData()));
            } else {
                // store the library in a variable

                // create heap value for the library
                HeapValue *ptr = params.state->HeapAlloc(params.thread);
                StackValue &res = params.thread->GetRegisters()[0];

                ASSERT(ptr != nullptr);

                // assign it to the library
                ptr->Assign(lib);

                // assign register value to the allocated object
                res.m_type = StackValue::HEAP_POINTER;
                res.m_value.ptr = ptr;
            }
        } else {
            params.state->ThrowException(params.thread, e);
        }
    } else {
        params.state->ThrowException(params.thread, e);
    }
}

void Runtime_load_function(ace::sdk::Params params)
{
    if (params.nargs != 2) {
        params.state->ThrowException(params.thread, Exception::InvalidArgsException(2, params.nargs));
        return;
    }

    StackValue *arg0 = params.args[0];
    ASSERT(arg0 != nullptr);

    StackValue *arg1 = params.args[1];
    ASSERT(arg1 != nullptr);

    Exception e = Exception(utf::Utf8String("load_function() expects arguments of type Library and String"));

    Library *libptr = nullptr;
    utf::Utf8String *strptr = nullptr;

    if (arg0->GetType() == StackValue::ValueType::HEAP_POINTER) {
        if (arg0->GetValue().ptr == nullptr) {
            params.state->ThrowException(params.thread, Exception::NullReferenceException());
        } else if ((libptr = arg0->GetValue().ptr->GetPointer<Library>()) == nullptr) {
            params.state->ThrowException(params.thread, e);
        } else {
            if (arg1->GetType() == StackValue::ValueType::HEAP_POINTER) {
                if (arg1->GetValue().ptr == nullptr) {
                    params.state->ThrowException(params.thread, Exception::NullReferenceException());
                } else if ((strptr = arg1->GetValue().ptr->GetPointer<utf::Utf8String>()) == nullptr) {
                    params.state->ThrowException(params.thread, e);
                } else {
                    NativeFunctionPtr_t func = libptr->GetFunction(strptr->GetData());
                    if (!func) {
                        // could not load function from the library
                        params.state->ThrowException(params.thread, Exception::LibraryFunctionLoadException(strptr->GetData()));
                    } else {
                        StackValue &res = params.thread->GetRegisters()[0];
                        res.m_type = StackValue::NATIVE_FUNCTION;
                        res.m_value.native_func = func;
                    }
                }
            } else {
                params.state->ThrowException(params.thread, e);
            }
        }
    } else {
        params.state->ThrowException(params.thread, e);
    }
}

void Global_to_string(ace::sdk::Params params)
{
    if (params.nargs != 1) {
        params.state->ThrowException(params.thread, Exception::InvalidArgsException(1, params.nargs));
        return;
    }

    // create heap value for string
    HeapValue *ptr = params.state->HeapAlloc(params.thread);
    StackValue &res = params.thread->GetRegisters()[0];
    
    ASSERT(ptr != nullptr);

    ptr->Assign(params.args[0]->ToString());
    // assign register value to the allocated object
    res.m_type = StackValue::HEAP_POINTER;
    res.m_value.ptr = ptr;
}

void Global_length(ace::sdk::Params params)
{
    if (params.nargs != 1) {
        params.state->ThrowException(params.thread,
            Exception::InvalidArgsException(1, params.nargs));
        return;
    }

    int len = 0;

    StackValue *target_ptr = params.args[0];
    ASSERT(target_ptr != nullptr);

    const int buffer_size = 256;
    char buffer[buffer_size];
    std::snprintf(buffer, buffer_size, "length() is undefined for type '%s'",
        target_ptr->GetTypeString());
    Exception e = Exception(utf::Utf8String(buffer));

    if (target_ptr->GetType() == StackValue::ValueType::HEAP_POINTER) {
        utf::Utf8String *strptr = nullptr;
        Array *arrayptr = nullptr;
        Object *objptr = nullptr;
        
        if (target_ptr->GetValue().ptr == nullptr) {
            params.state->ThrowException(params.thread, Exception::NullReferenceException());
        } else if ((strptr = target_ptr->GetValue().ptr->GetPointer<utf::Utf8String>()) != nullptr) {
            // get length of string
            len = strptr->GetLength();
        } else if ((arrayptr = target_ptr->GetValue().ptr->GetPointer<Array>()) != nullptr) {
            // get length of array
            len = arrayptr->GetSize();
        } else if ((objptr = target_ptr->GetValue().ptr->GetPointer<Object>()) != nullptr) {
            // get number of members in object
            len = objptr->GetSize();
        } else {
            params.state->ThrowException(params.thread, e);
        }
    } else {
        params.state->ThrowException(params.thread, e);
    }

    StackValue &res = params.thread->GetRegisters()[0];

    // assign register value to the length
    res.m_type = StackValue::I32;
    res.m_value.i32 = len;
}

void Global_spawn_thread(ace::sdk::Params params)
{
    if (params.nargs < 1) {
        params.state->ThrowException(params.thread,
            Exception::InvalidArgsException(1, params.nargs));
        return;
    }

    StackValue *target_ptr = params.args[0];
    ASSERT(target_ptr != nullptr);

    StackValue target(*target_ptr);

    const int buffer_size = 256;
    char buffer[buffer_size];
    std::snprintf(buffer, buffer_size, "spawn_thread() is undefined for type '%s'",
        target_ptr->GetTypeString());
    Exception e = Exception(utf::Utf8String(buffer));

    // the position of the bytecode stream before thread execution
    size_t pos = params.state->GetBytecodeStream()->Position();

    if (target.GetType() == StackValue::ValueType::FUNCTION) {
        // create the thread
        ExecutionThread *new_thread = params.state->CreateThread();
        ASSERT(new_thread != nullptr);

        // copy values to the new stack
        for (int i = 1; i < params.nargs; i++) {
            if (params.args[i] != nullptr) {
                new_thread->GetStack().Push(*params.args[i]);
            }
        }

        threads.emplace_back(std::thread([new_thread, params, target, pos]() {
            ASSERT(params.state->GetBytecodeStream() != nullptr);

            // create copy of byte stream
            BytecodeStream bs = *params.state->GetBytecodeStream();
            bs.SetPosition(pos);

            // keep track of function depth so we can
            // quit the thread when the function returns
            const int func_depth_start = new_thread->m_func_depth;
            
            // call the function
            params.state->m_vm->Invoke(new_thread, &bs, target, params.nargs - 1);

            while (!bs.Eof() && params.state->good && (new_thread->m_func_depth - func_depth_start)) {
                uint8_t code;
                bs.Read(&code, 1);

                params.state->m_vm->HandleInstruction(new_thread, &bs, code);
            }

            // remove the thread
            std::lock_guard<std::mutex> lock(mtx);
            params.state->DestroyThread(new_thread->GetId());
        }));
    } else {
        params.state->ThrowException(params.thread, e);
    }
}

static void LexLine(const utf::Utf8String &line, TokenStream &token_stream)
{
    CompilationUnit tmp_compilation_unit;

    // send code to be compiled
    SourceFile source_file("<stdin>", line.GetBufferSize());
    std::memcpy(source_file.GetBuffer(), line.GetData(), line.GetBufferSize());
    SourceStream source_stream(&source_file);

    Lexer lex(source_stream, &token_stream, &tmp_compilation_unit);
    lex.Analyze();
}

static int RunBytecodeFile(VM *vm, const utf::Utf8String &filename, bool record_time, int pos=0)
{
    ASSERT(vm != nullptr);

    threads.clear();

    // load bytecode from file
    std::ifstream file(filename.GetData(), std::ios::in | std::ios::binary | std::ios::ate);
    // file could not be opened
    if (!file.is_open()) {
        utf::cout << "Could not open file " << filename << "\n";
        return 1;
    }

    int64_t bytecode_size = file.tellg();
    file.seekg(0, std::ios::beg);

    ASSERT(bytecode_size > 0);

    char *bytecodes = new char[bytecode_size];
    file.read(bytecodes, bytecode_size);
    file.close();

    BytecodeStream bytecode_stream(non_owning_ptr<char>(bytecodes), (size_t)bytecode_size, pos);

    vm->GetState().SetBytecodeStream(non_owning_ptr<BytecodeStream>(&bytecode_stream));

    // time how long execution took
    auto start = std::chrono::high_resolution_clock::now();

    vm->Execute();

    if (record_time) {
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<
            std::chrono::duration<double, std::ratio<1>>>(end - start).count();
        utf::cout << "Elapsed time: " << elapsed_ms << "s\n";
    }

    // wait for threads
    for (std::thread &thread : threads) {
        thread.join();
    }
    threads.clear();

    delete[] bytecodes;

    return 0;
}

static int REPL(VM *vm, CompilationUnit &compilation_unit,
    const utf::Utf8String &template_code, bool first_time = true)
{
    AstIterator ast_iterator;

    int indent = 0;
    int parentheses_counter = 0;
    int bracket_counter = 0;

    utf::Utf8String code;
    utf::Utf8String out_filename = "tmp.aex";

    if (first_time) { // compile template code (expects module declaration)
        // truncate the file, overwriting any previous data
        std::ofstream out_file(out_filename.GetData(),
            std::ios::out | std::ios::binary | std::ios::trunc);
        out_file.close();

        code = template_code;
        utf::cout << code;

        // send code to be compiled
        SourceFile source_file("<stdin>", code.GetBufferSize());
        std::memcpy(source_file.GetBuffer(), code.GetData(), code.GetBufferSize());
        SourceStream source_stream(&source_file);

        TokenStream token_stream;
        Lexer lex(source_stream, &token_stream, &compilation_unit);
        lex.Analyze();

        Parser parser(&ast_iterator, &token_stream, &compilation_unit);
        parser.Parse(true);

        SemanticAnalyzer semantic_analyzer(&ast_iterator, &compilation_unit);
        semantic_analyzer.Analyze(true);
    }

    utf::Utf8String line;
    std::string tmp_line;

    utf::cout << "> ";
    while (std::getline(std::cin, tmp_line)) {
        utf::Utf8String current_line(tmp_line.c_str());

        if (current_line == "quit" || current_line == "quit()") {
            break;
        }

        for (char ch : tmp_line) {
            if (ch == '{') {
                indent++;
            } else if (ch == '}') {
                indent--;
            } else if (ch == '(') {
                parentheses_counter++;
            } else if (ch == ')') {
                parentheses_counter--;
            } else if (ch == '[') {
                bracket_counter++;
            } else if (ch == ']') {
                bracket_counter--;
            }
        }

        // run lexer on entered line to determine
        // if we should keep reading input
        TokenStream tmp_ts;
        LexLine(current_line, tmp_ts);

        bool cont_token = !tmp_ts.m_tokens.empty() && tmp_ts.m_tokens.back().IsContinuationToken();
        bool wait_for_next = indent > 0 ||
                parentheses_counter > 0 ||
                bracket_counter > 0 ||
                cont_token;

        line += current_line + "\n";

        if (!wait_for_next) {
            // so we can rewind upon errors
            int old_pos = ast_iterator.GetPosition();
            // store the number of identifiers in the global scope,
            // so we can remove them on error
            
            compilation_unit.m_module_index++;
            
            ASSERT(compilation_unit.GetCurrentModule() != nullptr);
            ASSERT(compilation_unit.GetCurrentModule()->m_scopes.TopNode() != nullptr);
            
            size_t num_identifiers = compilation_unit.GetCurrentModule()->m_scopes.Top()
                .GetIdentifierTable().GetIdentifiers().size();
            
            compilation_unit.m_module_index--;

            // send code to be compiled
            SourceFile source_file("<stdin>", line.GetBufferSize());
            source_file.ReadIntoBuffer(line.GetData(), line.GetBufferSize());
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
                
                std::string filename = error.GetLocation().GetFileName();
                std::string error_text = error.GetText();
                
                utf::cout
                        << utf::Utf8String(filename.c_str()) << " "
                        << "[" << (error.GetLocation().GetLine() + 1)
                        << ", " << (error.GetLocation().GetColumn() + 1)
                        << "]: " << utf::Utf8String(error_text.c_str()) << "\n";
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
                std::ofstream temp_bytecode_file(out_filename.GetData(),
                    std::ios::out | std::ios::binary | std::ios::app | std::ios::ate);

                int64_t file_pos = temp_bytecode_file.tellp();

                if (!temp_bytecode_file.is_open()) {
                    utf::cout << "Could not open file for writing: " << out_filename << "\n";
                    for (int i = old_pos; i < ast_iterator.GetPosition(); i++) {
                        ast_iterator.Pop();
                    }
                    ast_iterator.SetPosition(old_pos);
                } else {
                    int64_t bytecode_file_size;

                    temp_bytecode_file << compilation_unit.GetInstructionStream();
                    compilation_unit.GetInstructionStream().ClearInstructions();
                    bytecode_file_size = temp_bytecode_file.tellp();
                    temp_bytecode_file.close();

                    // check if we even have to run the bytecode file
                    if (bytecode_file_size - file_pos > 0) {

                        ASSERT(vm->GetState().GetNumThreads() > 0);
                        ExecutionThread *main_thread = vm->GetState().m_threads[0];
                        ASSERT(main_thread != nullptr);

                        // store stack size so that we can revert on error
                        size_t stack_size_before = main_thread->GetStack().GetStackPointer();

                        RunBytecodeFile(vm, out_filename, false, file_pos);

                        if (!vm->GetState().good) {
                            // if an exception was unhandled go back to previous state
                            // overwrite the bytecode file with the code that has been generated up to the point of error
                            // start by loading it into a temporary buffer
                            std::ifstream tmp_is(out_filename.GetData(),
                                std::ios::in | std::ios::binary | std::ios::ate);
                            // len is only the amount of bytes up to where we were before in the file.
                            int64_t len = std::min((int64_t) tmp_is.tellg(), file_pos);
                            // create buffer
                            char *buf = new char[len];
                            // seek to beginning
                            tmp_is.seekg(0, std::ios::beg);
                            // read into buffer
                            tmp_is.read(buf, len);
                            // close temporary file
                            tmp_is.close();

                            // create the new file to write the buffer to
                            std::ofstream tmp_os(out_filename.GetData(), std::ios::out | std::ios::binary);
                            tmp_os.write(buf, len);
                            tmp_os.close();

                            // delete buffer, it's no longer needed
                            delete[] buf;

                            size_t stack_size_now = main_thread->GetStack().GetStackPointer();
                            while (stack_size_now > stack_size_before) {
                                main_thread->GetStack().Pop();
                                stack_size_now--;
                            }

                            // kill off any thread that isn't the main thread
                            for (int i = 1; i < vm->GetState().GetNumThreads(); i++) {
                                if (vm->GetState().m_threads[i] != nullptr) {
                                    vm->GetState().DestroyThread(i);
                                }
                            }

                            // trigger the GC after popping items from stack,
                            // to clean up any heap variables no longer in use.
                            main_thread->GetStack().MarkAll();
                            vm->GetState().GetHeap().Sweep();

                            // clear vm exception state
                            main_thread->GetExceptionState().Reset();

                            // we're good to go now
                            vm->GetState().good = true;

                            // restart the script
                            return REPL(vm, compilation_unit, "", false);
                        } else {
                            // everything is good, no compile or runtime errors here
                            // store the line
                            code += line;
                        }
                    }
                }
            } else {
                // remove the identifiers that were since declared
                compilation_unit.m_module_index++;
                
                ASSERT(compilation_unit.GetCurrentModule() != nullptr);
                ASSERT(compilation_unit.GetCurrentModule()->m_scopes.TopNode() != nullptr);
                
                auto &tbl = compilation_unit.GetCurrentModule()->m_scopes.Top().GetIdentifierTable();
                auto &idents = tbl.GetIdentifiers();
                
                size_t diff = idents.size() - num_identifiers;
                
                for (size_t i = 0; i < diff; i++) {
                    tbl.PopIdentifier();
                }
                
                compilation_unit.m_module_index--;
                
                for (int i = old_pos; i < ast_iterator.GetPosition(); i++) {
                    ast_iterator.Pop();
                }
                ast_iterator.SetPosition(old_pos);
                compilation_unit.GetErrorList().ClearErrors();
            }

            line = "";

            utf::cout << "> ";
        } else {
            if (indent) {
                for (int i = 0; i < indent; i++) {
                    utf::cout << "..";
                }
            } else {
                if (parentheses_counter || bracket_counter || cont_token) {
                    utf::cout << "..";
                }
            }
            utf::cout << "  ";
        }
    }

    return 0;
}

void OnExit() {
    utf::cout << ("exited\n");
    utf::cout.flush();
}

int main(int argc, char *argv[])
{
    utf::init();

    VM vm;
    CompilationUnit compilation_unit;
    APIInstance api;

    api.Module("Runtime")
        .Function("gc", ObjectType::type_builtin_void, {}, Runtime_gc)
        .Function("load_library", ObjectType::type_builtin_any, {}, Runtime_load_library)
        .Function("load_function", ObjectType::type_builtin_any, {}, Runtime_load_function)
        .Variable("version", ObjectType::type_builtin_string, [](VMState *state, ExecutionThread *thread, StackValue *out) {
            ASSERT(state != nullptr);
            ASSERT(out != nullptr);

            // allocate heap value
            HeapValue *hv = state->HeapAlloc(thread);

            ASSERT(hv != nullptr);

            // create each version object
            StackValue sv_major;
            sv_major.m_type = StackValue::I32;
            sv_major.m_value.i32 = Runtime::VERSION_MAJOR;

            StackValue sv_minor;
            sv_minor.m_type = StackValue::I32;
            sv_minor.m_value.i32 = Runtime::VERSION_MINOR;

            StackValue sv_patch;
            sv_patch.m_type = StackValue::I32;
            sv_patch.m_value.i32 = Runtime::VERSION_PATCH;

            // create array
            Array res(3);
            res.AtIndex(0) = sv_major;
            res.AtIndex(1) = sv_minor;
            res.AtIndex(2) = sv_patch;

            // assign heap value to array
            hv->Assign(res);

            // assign the out value to this
            out->m_type = StackValue::HEAP_POINTER;
            out->m_value.ptr = hv;
        });

    api.Module(ace::compiler::Config::GLOBAL_MODULE_NAME)
        .Function("to_string", ObjectType::type_builtin_string, {}, Global_to_string)
        .Function("length", ObjectType::type_builtin_int, {}, Global_length)
        .Function("spawn_thread", ObjectType::type_builtin_void, {}, Global_spawn_thread);

    api.Module(ace::compiler::Config::GLOBAL_MODULE_NAME)
        .Type("Thread")
        .Method("start", ObjectType::type_builtin_void, {}, [](ace::sdk::Params) {
            utf::cout << "Thread.start called\n";
        });

    api.BindAll(&vm, &compilation_unit);

    if (argc == 1) {
        // trigger the REPL when no command line arguments have been provided
        // disable static objects, so we can append the bytecode files.
        ace::compiler::Config::use_static_objects = false;
        // do not cull unused objects
        ace::compiler::Config::cull_unused_objects = false;

        REPL(&vm, compilation_unit, "module repl    // to exit, type `quit`\n");

    } else if (argc >= 2) {
        enum {
            COMPILE_SOURCE,
            DECOMPILE_BYTECODE,
        } mode = COMPILE_SOURCE;

        utf::Utf8String src_filename;
        utf::Utf8String out_filename;

        if (CLI::HasOption(argv, argv + argc, "-d")) {
            // disassembly mode
            mode = DECOMPILE_BYTECODE;
            src_filename = CLI::GetOptionValue(argv, argv + argc, "-d");

            if (CLI::HasOption(argv, argv + argc, "-o")) {
                out_filename = CLI::GetOptionValue(argv, argv + argc, "-o");
            }
        } else {
            mode = COMPILE_SOURCE;

            if (CLI::HasOption(argv, argv + argc, "-c")) {
                src_filename = CLI::GetOptionValue(argv, argv + argc, "-c");
            }

            if (src_filename == "") {
                src_filename = argv[1];
            }

            if (CLI::HasOption(argv, argv + argc, "-o")) {
                out_filename = CLI::GetOptionValue(argv, argv + argc, "-o");
            }

            if (out_filename == "") {
                out_filename = (str_util::strip_extension(src_filename.GetData()) + ".aex").c_str();
            }
        }

        if (mode == COMPILE_SOURCE) {
            if (ace_compiler::BuildSourceFile(src_filename, out_filename, compilation_unit)) {
                // execute the compiled bytecode file
                RunBytecodeFile(&vm, out_filename, true);
            }
        } else if (mode == DECOMPILE_BYTECODE) {
            ace_compiler::DecompileBytecodeFile(src_filename, out_filename);
        }
    }
}
