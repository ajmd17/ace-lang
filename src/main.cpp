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

#include <iostream>
#include <string>

int main()
{
    CompilationUnit compilation_unit;

    TokenStream *token_stream = new TokenStream();

    SourceFile *src_file = new SourceFile(256);
    (*src_file) >> "module main \"Hello world\" /*blah*/ test + 4";

    SourceStream src_stream(src_file);

    Lexer lex(src_stream, token_stream, &compilation_unit);
    lex.Analyze();
    
    delete src_file;

    /*for (auto &it : token_stream->m_tokens) {
        std::cout << "{ " 
        << it.GetType() << ", "
        << it.GetValue() << " }\n";
    }*/

    AstIterator ast_iterator;
    Parser parser(token_stream, &compilation_unit, &ast_iterator);
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
        ast_iterator.ResetPosition();

        // optimization step
        Optimizer optimizer(&ast_iterator, &compilation_unit);
        optimizer.Optimize();
    }


    /*AstIterator ast_iterator;
    
    ast_iterator.Push(std::shared_ptr<AstModuleDeclaration>(
        new AstModuleDeclaration("mymodule", SourceLocation(0, 0, "blah.ar"))));
    
    std::shared_ptr<AstNull> true_value(new AstNull(SourceLocation(8, 9, "blah.ar")));
    ast_iterator.Push(std::shared_ptr<AstVariableDeclaration>(
        new AstVariableDeclaration("myvar", true_value, SourceLocation(1, 4, "blah.ar"))));
    
    ast_iterator.Push(std::shared_ptr<AstVariable>(
        new AstVariable("myvar", SourceLocation(1, 4, "blah.ar"))));

    //std::shared_ptr<AstVariable> left(new AstVariable("constvar", SourceLocation(5, 6, "blah.ar")));
    //std::shared_ptr<AstVariable> right(new AstVariable("nonconstvar", SourceLocation(5, 9, "blah.ar")));
    //ast_iterator.Push(std::shared_ptr<AstBinaryExpression>(
    //    new AstBinaryExpression(left, right, &Operator::operator_assign, SourceLocation(1, 4, "blah.ar"))));

    std::shared_ptr<AstBlock> block_scope(new AstBlock(SourceLocation(8, 9, "blah.ar")));
    ast_iterator.Push(std::shared_ptr<AstIfStatement>(
        new AstIfStatement(true_value, block_scope, SourceLocation(8, 9, "blah.ar"))));

    CompilationUnit compilation_unit;

    SemanticAnalyzer semantic_analyzer(ast_iterator, &compilation_unit);
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
        ast_iterator.ResetPosition();

        // optimization step
        Optimizer optimizer(ast_iterator, &compilation_unit);
        optimizer.Optimize();
    }*/

    std::cin.get();
    return 0;
}