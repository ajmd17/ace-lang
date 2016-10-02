#ifndef PARSER_H
#define PARSER_H

#include <athens/token_stream.h>
#include <athens/source_location.h>
#include <athens/compilation_unit.h>
#include <athens/ast_iterator.h>
#include <athens/keywords.h>

#include <string>

class Parser {
public:
    Parser(TokenStream *token_stream, CompilationUnit *compilation_unit, 
        AstIterator *ast_iterator);
    Parser(const Parser &other);

    /** Generate an AST structure from the token stream */
    void Parse();

private:
    TokenStream *m_token_stream;
    CompilationUnit *m_compilation_unit;
    AstIterator *m_ast_iterator;

    const Token *Match(TokenType type, bool read = false);
    const Token *MatchKeyword(Keywords keyword, bool read = false);
    const Token *Expect(TokenType type, bool read = false);
    const Token *ExpectKeyword(Keywords keyword, bool read = false);
    const SourceLocation &CurrentLocation() const;
};

#endif