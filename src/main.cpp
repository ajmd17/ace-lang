#include <athens/module.h>
#include <athens/semantic_analyzer.h>
#include <athens/compilation_unit.h>
#include <athens/ast/ast_module_declaration.h>
#include <athens/emit/instruction.h>
#include <athens/stack_tree.h>

#include <iostream>

int main()
{
    Tree<int> *mytree = new Tree<int>();
    mytree->Open(4);
    mytree->Open(2);
    mytree->Close();
    mytree->Close();
    mytree->Open(7);
    mytree->Close();
    mytree->operator<<(std::cout);
    delete mytree;


    AstModuleDeclaration *module_declaration = new AstModuleDeclaration("mymodule", SourceLocation(0, 0, "blah"));
    AstModuleDeclaration *module_declaration2 = new AstModuleDeclaration("mymodule", SourceLocation(5, 4, "blah"));

    AstIterator ast_iterator;
    ast_iterator.PushBack(module_declaration);

    AstIterator ast_iterator2;
    ast_iterator2.PushBack(module_declaration2);

    CompilationUnit compilation_unit;

    SemanticAnalyzer semantic_analyzer(ast_iterator, &compilation_unit);
    semantic_analyzer.Analyze();

    SemanticAnalyzer semantic_analyzer2(ast_iterator2, &compilation_unit);
    semantic_analyzer2.Analyze();

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

    delete module_declaration;
    delete module_declaration2;

    std::cin.get();
    return 0;
}