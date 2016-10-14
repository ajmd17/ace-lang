#ifndef LEXER_H
#define LEXER_H

#include <athens/source_stream.h>
#include <athens/token_stream.h>
#include <athens/source_location.h>
#include <athens/compilation_unit.h>

#include <common/utf8.h>

class Lexer {
public:
    Lexer(const SourceStream &source_stream, TokenStream *token_stream, 
        CompilationUnit *compilation_unit);
    Lexer(const Lexer &other);

    /** Forms the given TokenStream from the given SourceStream */
    void Analyze();
    /** Reads the next token and returns it */
    Token NextToken();
    /** Reads two characters that make up an escape code and returns actual value */
    u32char ReadEscapeCode();
    /** Reads a string literal and returns the token */
    Token ReadStringLiteral();
    /** Reads a number literal and returns the token */
    Token ReadNumberLiteral();
    /** Reads a hex number literal and returns the token */
    Token ReadHexNumberLiteral();
    /** Reads a single-line comment */
    Token ReadLineComment();
    /** Reads a multi-line block comment */
    Token ReadBlockComment();
    /** Reads an important comment (documentation block) */
    Token ReadDocumentation();
    /** Reads an operator and returns the token */
    Token ReadOperator();
    /** Reads the name, and returns the either identifier or keyword token */
    Token ReadIdentifier();

private:
    SourceStream m_source_stream;
    TokenStream *m_token_stream;
    CompilationUnit *m_compilation_unit;
    SourceLocation m_source_location;

    /** Adds an end-of-file error if at the end, returns true if not */
    bool HasNext();
    void SkipWhitespace();
};

#endif