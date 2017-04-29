#include <ace-c/ace-c.hpp>

#include <ace-c/Module.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/Optimizer.hpp>
#include <ace-c/ast/AstModuleDeclaration.hpp>
#include <ace-c/emit/Instruction.hpp>
#include <ace-c/Lexer.hpp>
#include <ace-c/Parser.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/Minifier.hpp>
#include <ace-c/dis/DecompilationUnit.hpp>
#include <ace-c/dis/ByteStream.hpp>

#include <common/str_util.hpp>

#include <fstream>
#include <unordered_set>

namespace ace_compiler {

bool BuildSourceFile(const utf::Utf8String &filename,
    const utf::Utf8String &out_filename)
{
    CompilationUnit compilation_unit;
    return BuildSourceFile(filename, out_filename, compilation_unit);
}

bool BuildSourceFile(const utf::Utf8String &filename,
    const utf::Utf8String &out_filename, CompilationUnit &compilation_unit)
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
        TokenStream token_stream;

        Lexer lex(source_stream, &token_stream, &compilation_unit);
        lex.Analyze();

        AstIterator ast_iterator;
        Parser parser(&ast_iterator, &token_stream, &compilation_unit);
        parser.Parse();

        SemanticAnalyzer semantic_analyzer(&ast_iterator, &compilation_unit);
        semantic_analyzer.Analyze();

        compilation_unit.GetErrorList().SortErrors();

        std::unordered_set<std::string> error_filenames;

        for (CompilerError &error : compilation_unit.GetErrorList().m_errors) {
            if (error_filenames.insert(error.GetLocation().GetFileName()).second) {
                auto split = str_util::split_path(error.GetLocation().GetFileName());
                std::string str = !split.empty() ? split.back() : error.GetLocation().GetFileName();
                str = str_util::strip_extension(str);

                utf::cout << "In file \"" << utf::Utf8String(str.c_str()) << "\":\n";
            }

            utf::cout << "  "
                      << "Ln "    << (error.GetLocation().GetLine() + 1)
                      << ", Col " << (error.GetLocation().GetColumn() + 1)
                      << ":  "    << utf::Utf8String(error.GetText().c_str()) << "\n";
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

            return true;
        }
    }

    return false;
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

        DecompilationUnit decompilation_unit;

        bool write_to_file = false;
        utf::utf8_ostream *os = nullptr;
        if (out_filename == "") {
            os = &utf::cout;
        } else {
            write_to_file = true;
            os = new utf::utf8_ofstream(out_filename.GetData(), std::ios::out | std::ios::binary);
        }

        InstructionStream instruction_stream = decompilation_unit.Decompile(bs, os);

        if (write_to_file) {
            delete os;
        }
    }
}

} // ace_compiler
