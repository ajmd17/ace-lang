#include <ace-c/ace-c.hpp>

#include <ace-c/module.hpp>
#include <ace-c/semantic_analyzer.hpp>
#include <ace-c/optimizer.hpp>
#include <ace-c/compilation_unit.hpp>
#include <ace-c/ast/ast_module_declaration.hpp>
#include <ace-c/ast/ast_variable_declaration.hpp>
#include <ace-c/ast/ast_variable.hpp>
#include <ace-c/ast/ast_binary_expression.hpp>
#include <ace-c/ast/ast_if_statement.hpp>
#include <ace-c/ast/ast_true.hpp>
#include <ace-c/ast/ast_null.hpp>
#include <ace-c/ast/ast_block.hpp>
#include <ace-c/emit/instruction.hpp>
#include <ace-c/lexer.hpp>
#include <ace-c/parser.hpp>
#include <ace-c/compiler.hpp>
#include <ace-c/dis/decompilation_unit.hpp>
#include <ace-c/dis/byte_stream.hpp>

#include <fstream>

namespace ace_compiler {
void BuildSourceFile(const utf::Utf8String &filename, const utf::Utf8String &out_filename)
{
    std::ifstream in_file(filename.GetData(), std::ios::in | std::ios::ate | std::ios::binary);

    if (!in_file.is_open()) {
        utf::cout << "Could not open file: " << filename << "\n";
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
            utf::cout
                << utf::Utf8String(error.GetLocation().GetFileName().c_str()) << " "
                << "[" << (error.GetLocation().GetLine() + 1)
                << ", " << (error.GetLocation().GetColumn() + 1)
                << "]: " << utf::Utf8String(error.GetText().c_str()) << "\n";
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
                utf::cout << "Could not open file for writing: " << out_filename << "\n";
            } else {
                out_file << compilation_unit.GetInstructionStream();
            }
            out_file.close();
        }
    }
}

void DecompileBytecodeFile(const utf::Utf8String &filename, const utf::Utf8String &out_filename)
{
    std::ifstream in_file(filename.GetData(), std::ios::in | std::ios::ate | std::ios::binary);

    if (!in_file.is_open()) {
        utf::cout << "Could not open file: " << filename << "\n";
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
            os = &utf::cout;
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
} // ace_compiler
