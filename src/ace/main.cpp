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

#include <cstdio>
#include <cstdlib>

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

        utf::Utf8String out_filename = "tmp.aex";
        std::ofstream out_file(out_filename.GetData(),
            std::ios::out | std::ios::binary | std::ios::trunc);
        out_file.close();

        int block_counter = 0;
        std::string code = "// Ace REPL\nmodule repl;\n";
        utf::cout << code;

        { // compile template code
            // send code to be compiled
            SourceFile source_file("<cli>", code.length());
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
                SourceFile source_file("<cli>", line.length());
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
