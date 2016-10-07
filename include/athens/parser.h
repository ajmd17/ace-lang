#ifndef PARSER_H
#define PARSER_H

#include <athens/token_stream.h>
#include <athens/source_location.h>
#include <athens/compilation_unit.h>
#include <athens/ast_iterator.h>
#include <athens/keywords.h>
#include <athens/ast/ast_module_declaration.h>
#include <athens/ast/ast_variable_declaration.h>
#include <athens/ast/ast_statement.h>
#include <athens/ast/ast_expression.h>
#include <athens/ast/ast_import.h>
#include <athens/ast/ast_local_import.h>
#include <athens/ast/ast_integer.h>
#include <athens/ast/ast_float.h>
#include <athens/ast/ast_string.h>
#include <athens/ast/ast_binary_expression.h>
#include <athens/ast/ast_function_call.h>
#include <athens/ast/ast_variable.h>
#include <athens/ast/ast_true.h>
#include <athens/ast/ast_false.h>
#include <athens/ast/ast_null.h>
#include <athens/ast/ast_block.h>

#include <string>

class Parser {
public:
    Parser(AstIterator *ast_iterator, TokenStream *token_stream, 
        CompilationUnit *compilation_unit);
    Parser(const Parser &other);

    /** Generate an AST structure from the token stream */
    void Parse();

private:
    AstIterator *m_ast_iterator;
    TokenStream *m_token_stream;
    CompilationUnit *m_compilation_unit;

    const Token *MatchAhead(TokenType type, int n);
    const Token *Match(TokenType type, bool read = false);
    const Token *MatchKeyword(Keywords keyword, bool read = false);
    const Token *Expect(TokenType type, bool read = false);
    const Token *ExpectKeyword(Keywords keyword, bool read = false);
    const SourceLocation &CurrentLocation() const;

    int OperatorPrecedence(const Operator *&out);

    std::shared_ptr<AstStatement> ParseStatement();
    std::shared_ptr<AstExpression> ParseTerm();
    std::shared_ptr<AstExpression> ParseParentheses();
    std::shared_ptr<AstInteger> ParseIntegerLiteral();
    std::shared_ptr<AstFloat> ParseFloatLiteral();
    std::shared_ptr<AstString> ParseStringLiteral();
    std::shared_ptr<AstExpression> ParseIdentifier();
    std::shared_ptr<AstFunctionCall> ParseFunctionCall();
    std::shared_ptr<AstTrue> ParseTrue();
    std::shared_ptr<AstFalse> ParseFalse();
    std::shared_ptr<AstNull> ParseNull();
    std::shared_ptr<AstBlock> ParseBlock();
    std::shared_ptr<AstExpression> ParseBinaryExpression(int expr_prec, 
        std::shared_ptr<AstExpression> left);
    std::shared_ptr<AstExpression> ParseExpression(bool standalone = false);
    std::shared_ptr<AstVariableDeclaration> ParseVariableDeclaration();
    std::shared_ptr<AstImport> ParseImport();
    std::shared_ptr<AstLocalImport> ParseLocalImport();
};

#endif