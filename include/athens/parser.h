#ifndef PARSER_H
#define PARSER_H

#include <athens/token_stream.h>
#include <athens/source_location.h>
#include <athens/compilation_unit.h>
#include <athens/ast_iterator.h>
#include <athens/keywords.h>
#include <athens/ast/ast_statement.h>
#include <athens/ast/ast_expression.h>
#include <athens/ast/ast_import.h>
#include <athens/ast/ast_local_import.h>

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

    const Token *Match(TokenType type, bool read = false);
    const Token *MatchKeyword(Keywords keyword, bool read = false);
    const Token *Expect(TokenType type, bool read = false);
    const Token *ExpectKeyword(Keywords keyword, bool read = false);
    const SourceLocation &CurrentLocation() const;

    std::shared_ptr<AstStatement> ParseStatement();
    std::shared_ptr<AstExpression> ParseExpression(bool standalone = false);
    std::shared_ptr<AstImport> ParseImport();
    std::shared_ptr<AstLocalImport> ParseLocalImport();
};

#endif