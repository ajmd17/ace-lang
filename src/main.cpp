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

int main()
{
    const char *filename = "bytecode.bin";

    CompilationUnit compilation_unit;

    TokenStream *token_stream = new TokenStream();

    SourceFile *src_file = new SourceFile(512);
    (*src_file) >> "module main (2 * (4 + (4 * (9 + 2 * 9) + (3+5)))) + (3 * (8 + 4)) * 9 + 10";

    SourceStream src_stream(src_file);

    Lexer lex(src_stream, token_stream, &compilation_unit);
    lex.Analyze();

    delete src_file;

    AstIterator ast_iterator;
    Parser parser(&ast_iterator, token_stream, &compilation_unit);
    parser.Parse();

    delete token_stream;

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
        std::ofstream out(filename, std::ios::out | std::ios::binary);
        out << compilation_unit.GetInstructionStream();
        out.close();
    }

    std::cin.get();
    return 0;
}