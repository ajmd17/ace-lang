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

#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char *argv[])
{
    if (argc == 1) {
        std::cout << "\tUsage: " << argv[0] << " <file>\n";

    } else if (argc >= 2) {

        const char *in_filename = argv[1];
        const char *out_filename = "bytecode.bin";


        std::ifstream in_file(in_filename, std::ios::in | std::ios::ate);
        if (!in_file.is_open()) {
            std::cout << "Could not open file: " << in_filename << "\n";
        } else {
            // get number of bytes
            size_t max = in_file.tellg();
            // seek to beginning
            in_file.seekg(0, std::ios::beg);
            // load stream into file buffer
            SourceFile source_file(in_filename, max);
            in_file.read(source_file.GetBuffer(), max);
            in_file.close();

            SourceStream source_stream(&source_file);


            CompilationUnit compilation_unit;

            TokenStream token_stream;

           /* SourceFile *src_file = new SourceFile("main_file", 512);
            (*src_file) >> "module main\n"
                           "import \"blah.txt\";\n"
                           "var a = 2;\n"
                           "var b = 3;\n"
                           "{ var x = 99; x + 3; a + 4; }\n"
                           "a + b * 3000;\n";
                           //"b + (2 * (4 + (4 * (9 + 2 * 9) + (3+5)))) + (3 * (8 + 4)) * 9 + 10\n";*/

            Lexer lex(source_stream, &token_stream, &compilation_unit);
            lex.Analyze();

            AstIterator ast_iterator;
            Parser parser(&ast_iterator, &token_stream, &compilation_unit);
            parser.Parse();

            SemanticAnalyzer semantic_analyzer(&ast_iterator, &compilation_unit);
            semantic_analyzer.Analyze();

            compilation_unit.GetErrorList().SortErrors();
            for (CompilerError &error : compilation_unit.GetErrorList().m_errors) {
                std::cout
                    << error.GetLocation().GetFileName() << "\t"
                    << "ln: "  << (error.GetLocation().GetLine() + 1) 
                    << ", col: " << (error.GetLocation().GetColumn() + 1) 
                    << ":\t" << error.GetText() << "\n";
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
                std::ofstream out(out_filename, std::ios::out | std::ios::binary);
                out << compilation_unit.GetInstructionStream();
                out.close();
            }
        }
    }

    std::cin.get();
    return 0;
}