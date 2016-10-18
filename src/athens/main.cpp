#include <athens/module.h>
#include <athens/semantic_analyzer.h>
#include <athens/optimizer.h>
#include <athens/compilation_unit.h>
#include <athens/ast/ast_module_declaration.h>
#include <athens/ast/ast_variable_declaration.h>
#include <athens/ast/ast_variable.h>
#include <athens/ast/ast_binary_expression.h>
#include <athens/ast/ast_if_statement.h>
#include <athens/ast/ast_true.h>
#include <athens/ast/ast_null.h>
#include <athens/ast/ast_block.h>
#include <athens/emit/instruction.h>
#include <athens/lexer.h>
#include <athens/parser.h>
#include <athens/compiler.h>
#include <athens/dis/decompilation_unit.h>
#include <athens/dis/byte_stream.h>

#include <common/utf8.h>

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

/** check if the option is set */
inline bool has_option(char **begin, char **end, const std::string &opt) 
{
    return std::find(begin, end, opt) != end;
}

/** retrieve the value that is found after an option */
inline char *get_option_value(char **begin, char **end, const std::string &opt)
{
    char **it = std::find(begin, end, opt);
    if (it != end && ++it != end) {
        return *it;
    }
    return nullptr;
}

void build_source_file(const Utf8String &filename, const Utf8String &out_filename)
{
    std::ifstream in_file(filename.GetData(), std::ios::in | std::ios::ate | std::ios::binary);

    if (!in_file.is_open()) {
        ucout << "Could not open file: " << filename << "\n";
    } else {
        // get number of bytes
        size_t max = in_file.tellg();
        // seek to beginning
        in_file.seekg(0, std::ios::beg);
        // load stream into file buffer
        SourceFile source_file(filename.GetData(), max);
        in_file.read(source_file.GetBuffer(), max);
        in_file.close();

        SourceStream source_stream(&source_file);

        CompilationUnit compilation_unit;
        TokenStream token_stream;

        Lexer lex(source_stream, &token_stream, &compilation_unit);
        lex.Analyze();

        AstIterator ast_iterator;
        Parser parser(&ast_iterator, &token_stream, &compilation_unit);
        parser.Parse();

        SemanticAnalyzer semantic_analyzer(&ast_iterator, &compilation_unit);
        semantic_analyzer.Analyze();

        compilation_unit.GetErrorList().SortErrors();
        for (CompilerError &error : compilation_unit.GetErrorList().m_errors) {
            ucout
                << Utf8String(error.GetLocation().GetFileName().c_str()) << "\t"
                << "ln: "  << (error.GetLocation().GetLine() + 1) 
                << ", col: " << (error.GetLocation().GetColumn() + 1) 
                << ":\t" << Utf8String(error.GetText().c_str()) << "\n";
        }

        if (!compilation_unit.GetErrorList().HasFatalErrors()) {
            // only optimize if there were no errors
            // before this point
            ast_iterator.ResetPosition();
            Optimizer optimizer(&ast_iterator, &compilation_unit);
            optimizer.Optimize();

            // compile into bytecode instructions
            ast_iterator.ResetPosition();
            Compiler compiler(&ast_iterator, &compilation_unit);
            compiler.Compile();

            // emit bytecode instructions to file
            std::ofstream out_file(out_filename.GetData(), std::ios::out | std::ios::binary);
            if (!out_file.is_open()) {
                ucout << "Could not open file for writing: " << out_filename << "\n";
            } else {
                out_file << compilation_unit.GetInstructionStream();
            }
            out_file.close();
        }
    }
}

void decompile_bytecode_file(const Utf8String &filename, const Utf8String &out_filename)
{
    std::ifstream in_file(filename.GetData(), std::ios::in | std::ios::ate | std::ios::binary);

    if (!in_file.is_open()) {
        ucout << "Could not open file: " << filename << "\n";
    } else {
        // get number of bytes
        size_t max = in_file.tellg();
        // seek to beginning
        in_file.seekg(0, std::ios::beg);
        // load stream into file buffer
        SourceFile source_file(filename.GetData(), max);
        in_file.read(source_file.GetBuffer(), max);
        in_file.close();

        ByteStream bs(&source_file);

        DecompilationUnit decompilation_unit(bs);

        bool write_to_file = false;
        std::ostream *os = nullptr;
        if (out_filename == "") {
            os = &ucout;
        } else {
            write_to_file = true;
            os = new std::ofstream(out_filename.GetData(), std::ios::out | std::ios::binary);
        }

        InstructionStream instruction_stream = decompilation_unit.Decompile(os);

        if (write_to_file) {
            delete os;
        }
    }
}

int main(int argc, char *argv[])
{
    utf8_init();

    if (argc == 1) {
        ucout << "\tUsage: " << argv[0] << " <file>\n";

    } else if (argc >= 2) {
        enum {
            COMPILE_SOURCE,
            DECOMPILE_BYTECODE,
        } mode = COMPILE_SOURCE;

        Utf8String filename;
        Utf8String out_filename;

        if (has_option(argv, argv + argc, "-d")) {
            // disassembly mode
            mode = DECOMPILE_BYTECODE;
            filename = get_option_value(argv, argv + argc, "-d");

            if (has_option(argv, argv + argc, "-o")) {
                out_filename = get_option_value(argv, argv + argc, "-o");
            }
        } else {
            mode = COMPILE_SOURCE;

            if (has_option(argv, argv + argc, "-c")) {
                filename = get_option_value(argv, argv + argc, "-c");
            }

            if (filename == "") {
                filename = argv[1];
            }

            if (has_option(argv, argv + argc, "-o")) {
                out_filename = get_option_value(argv, argv + argc, "-o");
            }

            if (out_filename == "") {
                out_filename = "out.bin";
            }
        }

        if (mode == COMPILE_SOURCE) {
            build_source_file(filename, out_filename);
        } else if (mode == DECOMPILE_BYTECODE) {
            decompile_bytecode_file(filename, out_filename);
        }
    }

    return 0;
}