#include <athens/module.h>
#include <athens/semantic_analyzer.h>
#include <athens/compilation_unit.h>
#include <athens/ast/ast_module_declaration.h>
#include <athens/ast/ast_variable_declaration.h>
#include <athens/ast/ast_variable.h>
#include <athens/emit/instruction.h>

#include <iostream>

int main()
{
    auto *module_declaration = new AstModuleDeclaration("mymodule", SourceLocation(0, 0, "blah.ar"));
    auto *var_declaration = new AstVariableDeclaration("myvar", nullptr, SourceLocation(1, 4, "blah.ar"));
    auto *var_reference = new AstVariable("myvar", SourceLocation(1, 4, "blah.ar"));

    AstIterator ast_iterator;
    ast_iterator.PushBack(module_declaration);
    ast_iterator.PushBack(var_declaration);
    ast_iterator.PushBack(var_reference);

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
    }

    delete var_reference;
    delete var_declaration;
    delete module_declaration;

    std::cin.get();
    return 0;
}