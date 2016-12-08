#include <ace/api.hpp>

#include <ace-c/ace-c.hpp>
#include <ace-c/configuration.hpp>
#include <ace-c/semantic_analyzer.hpp>
#include <ace-c/optimizer.hpp>
#include <ace-c/lexer.hpp>
#include <ace-c/parser.hpp>
#include <ace-c/compiler.hpp>

#include <ace-vm/object.hpp>
#include <ace-vm/array.hpp>

#include <common/utf8.hpp>
#include <common/cli_args.hpp>
#include <common/str_util.hpp>
#include <common/my_assert.hpp>

#include <string>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

using namespace ace;

void Runtime_gc(VMState *state, StackValue **args, int nargs)
{
    /*std::stringstream ss;
    // dump heap to stringstream
    ss << "Before:\n" << state->m_heap << "\n\n";
    auto str = ss.str();
    utf::cout << utf::Utf8String(str.c_str());
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
    auto str = ss.str();
    utf::cout << utf::Utf8String(str.c_str());*/
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
    
    ASSERT(ptr != nullptr);

    ptr->Assign(args[0]->ToString());
    // assign register value to the allocated object
    res.m_type = StackValue::HEAP_POINTER;
    res.m_value.ptr = ptr;
}

void Global_length(VMState *state, StackValue **args, int nargs)
{
    if (nargs != 1) {
        state->ThrowException(Exception::InvalidArgsException(1, nargs));
        return;
    }


    int len = 0;

    StackValue *target_ptr = args[0];
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
            state->ThrowException(Exception::NullReferenceException());
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
            state->ThrowException(e);
        }
    } else {
        state->ThrowException(e);
    }

    StackValue &res = state->GetExecutionThread().GetRegisters()[0];

    // assign register value to the length
    res.m_type = StackValue::INT32;
    res.m_value.i32 = len;
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

            // send code to be compiled
            SourceFile source_file("<stdin>", line.GetBufferSize());
            std::memcpy(source_file.GetBuffer(), line.GetData(), line.GetBufferSize());
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
                    temp_bytecode_file << compilation_unit.GetInstructionStream();
                    compilation_unit.GetInstructionStream().ClearInstructions();
                    temp_bytecode_file.close();

                    // store stack size so that we can revert on error
                    size_t stack_size_before = vm->GetState().GetExecutionThread().GetStack().GetStackPointer();

                    ace_vm::RunBytecodeFile(vm, out_filename, false, file_pos);

                    if (!vm->GetState().good) {
                        // if an exception was unhandled, ask the user if they'd like to restart.
                        utf::cout <<
                            "A runtime error has occurred within this script.\n"
                            "Would you like to go back to the previous state? Answering (N) will destroy the script.";

                        int response = -1;
                        while (response == -1) {
                            utf::cout << " (Y/n): ";

                            std::string str;
                            std::getline(std::cin, str);
                            str = str_util::trim(str);

                            if (!str.empty()) {
                                int ch = std::tolower(str[0]);
                                if (ch == 'y') {
                                    response = 1;
                                } else if (ch == 'n') {
                                    response = 0;
                                } else {
                                    utf::cout << "I'm sorry, I didn't catch that. I only understand yes and no.";
                                    response = -1;
                                }
                            }
                        }

                        if (response == 1) {
                            // overwrite the bytecode file with the code that has been generated up to the point of error
                            // start by loading it into a temporary buffer
                            std::ifstream tmp_is(out_filename.GetData(), std::ios::in | std::ios::binary | std::ios::ate);
                            // len is only the amount of bytes up to where we were before in the file.
                            int64_t len = std::min((int64_t)tmp_is.tellg(), file_pos);
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

                            size_t stack_size_now = vm->GetState().GetExecutionThread().GetStack().GetStackPointer();
                            while (stack_size_now > stack_size_before) {
                                vm->GetState().GetExecutionThread().GetStack().Pop();
                                stack_size_now--;
                            }

                            // trigger the GC after popping items from stack,
                            // to clean up any heap variables no longer in use.
                            vm->GetState().GetExecutionThread().GetStack().MarkAll();
                            vm->GetState().GetHeap().Sweep();

                            // clear vm exception state
                            vm->GetState().GetExecutionThread().GetExceptionState().Reset();

                            // we're good to go now
                            vm->GetState().good = true;

                            // restart the script
                            return REPL(vm, compilation_unit, "", false);
                        } else {
                            // user selected no
                            return 1;
                        }
                    } else {
                        // everything is good, no compile or runtime errors here
                        // store the line
                        code += line;
                    }
                }
            } else {
                for (int i = old_pos; i < ast_iterator.GetPosition(); i++) {
                    ast_iterator.Pop();
                }
                ast_iterator.SetPosition(old_pos);
                compilation_unit.GetErrorList().ClearErrors();
            }

            line = "";

            utf::cout << "> ";
        } else {
            if (wait_for_next) {
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
    }

    return 0;
}

int main(int argc, char *argv[])
{
    utf::init();

    VM *vm = new VM;
    CompilationUnit compilation_unit;

    APIInstance api;
    api.Function("Runtime", "gc", ObjectType::type_builtin_void, {}, Runtime_gc);
    api.Function(API::GLOBAL_MODULE_NAME, "to_string", ObjectType::type_builtin_string, {}, Global_to_string);
    api.Function(API::GLOBAL_MODULE_NAME, "length", ObjectType::type_builtin_int, {}, Global_length);
    api.BindAll(vm, &compilation_unit);

    if (argc == 1) {
        // trigger the REPL when no command line arguments have been provided
        // disable static objects, so we can append the bytecode files.
        ace::compiler::Config::use_static_objects = false;
        // do not cull unused objects
        ace::compiler::Config::cull_unused_objects = false;

        REPL(vm, compilation_unit, "module repl;\n");

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
                ace_vm::RunBytecodeFile(vm, out_filename, true);
            }
        } else if (mode == DECOMPILE_BYTECODE) {
            ace_compiler::DecompileBytecodeFile(src_filename, out_filename);
        }
    }

    delete vm;
}
