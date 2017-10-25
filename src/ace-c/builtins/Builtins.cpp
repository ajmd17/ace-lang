#include <ace-c/builtins/Builtins.hpp>
#include <ace-c/SourceFile.hpp>
#include <ace-c/SourceStream.hpp>
#include <ace-c/AstIterator.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/ast/AstTypeObject.hpp>
#include <ace-c/ast/AstVariableDeclaration.hpp>
#include <ace-c/type-system/BuiltinTypes.hpp>
#include <ace-c/SourceLocation.hpp>

const SourceLocation Builtins::BUILTIN_SOURCE_LOCATION(-1, -1, "<builtin>");

Builtins::Builtins()
{
    m_vars["Type"].reset(new AstTypeObject(
        BuiltinTypes::TYPE_TYPE, nullptr, SourceLocation::eof
    ));

    m_vars["Int"].reset(new AstTypeObject(
        BuiltinTypes::INT, nullptr, SourceLocation::eof
    ));

    m_vars["Float"].reset(new AstTypeObject(
        BuiltinTypes::FLOAT, nullptr, SourceLocation::eof
    ));

    m_vars["Number"].reset(new AstTypeObject(
        BuiltinTypes::NUMBER, nullptr, SourceLocation::eof
    ));

    m_vars["Bool"].reset(new AstTypeObject(
        BuiltinTypes::BOOLEAN, nullptr, SourceLocation::eof
    ));

    m_vars["String"].reset(new AstTypeObject(
        BuiltinTypes::STRING, nullptr, SourceLocation::eof
    ));

    m_vars["Function"].reset(new AstTypeObject(
        BuiltinTypes::FUNCTION, nullptr, SourceLocation::eof
    ));

    m_vars["Array"].reset(new AstTypeObject(
        BuiltinTypes::ARRAY, nullptr, SourceLocation::eof
    ));

    for (const auto &it : m_vars) {
        m_ast.Push(std::shared_ptr<AstVariableDeclaration>(new AstVariableDeclaration(
            it.first, nullptr, it.second, {}, true /* constant */, BUILTIN_SOURCE_LOCATION
        )));
    }
}

void Builtins::Visit(CompilationUnit *unit)
{
    SemanticAnalyzer analyzer(&m_ast, unit);
    analyzer.Analyze(false);

    m_ast.ResetPosition();
}

std::unique_ptr<BytecodeChunk> Builtins::Build(CompilationUnit *unit)
{
    Compiler compiler(&m_ast, unit);
    std::unique_ptr<BytecodeChunk> chunk = compiler.Compile();

    m_ast.ResetPosition();

    return std::move(chunk);
}