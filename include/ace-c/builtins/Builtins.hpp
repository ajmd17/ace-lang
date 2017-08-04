#ifndef BUILTINS_HPP
#define BUILTINS_HPP

#include <ace-c/SourceLocation.hpp>
#include <ace-c/CompilationUnit.hpp>
#include <ace-c/AstIterator.hpp>
#include <ace-c/ast/AstExpression.hpp>
#include <ace-c/emit/BytecodeChunk.hpp>

#include <map>
#include <string>
#include <memory>

class Builtins {
public:
    Builtins();

    inline const AstIterator &GetAst() const { return m_ast; }

    /** This will analyze the builtins, and add them to the syntax tree.
     */
    void Visit(CompilationUnit *unit);

    /** This will return bytecode containing builtins
     */
    std::unique_ptr<BytecodeChunk> Build(CompilationUnit *unit);

private:
    static const SourceLocation BUILTIN_SOURCE_LOCATION;

    std::map<std::string, std::shared_ptr<AstExpression>> m_vars;
    AstIterator m_ast;
};

#endif