#include <athens/lexer.h>
#include <athens/operator.h>
#include <athens/compiler_error.h>
#include <athens/keywords.h>

#include <array>
#include <sstream>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>

Lexer::Lexer(const SourceStream &source_stream, TokenStream *token_stream,
    CompilationUnit *compilation_unit)
    : m_source_stream(source_stream),
      m_token_stream(token_stream),
      m_compilation_unit(compilation_unit)
{
    m_source_location.SetFileName(m_source_stream.GetFile()->GetFilePath());
}

Lexer::Lexer(const Lexer &other)
    : m_source_stream(other.m_source_stream),
      m_token_stream(other.m_token_stream),
      m_compilation_unit(other.m_compilation_unit),
      m_source_location(other.m_source_location)
{
}

void Lexer::Analyze()
{
    SkipWhitespace();
    while (m_source_stream.HasNext() && m_source_stream.Peek() != '\0') {
        Token token = NextToken();
        if (!token.Empty()) {
            m_token_stream->Push(token);
        }
        SkipWhitespace();
    }
}

Token Lexer::NextToken()
{
    SourceLocation location(m_source_location);

    std::array<char, 3> ch; 
    for (int i = 0; i < 3; i++) {
        ch[i] = m_source_stream.Peek(i);
    }

    if (ch[0] == '\"') {
        return ReadStringLiteral();
    } else if (std::isdigit(ch[0]) || ch[0] == '.') {
        return ReadNumberLiteral();
    } else if (ch[0] == '0' && (ch[1] == 'x' || ch[1] == 'X')) {
        return ReadHexNumberLiteral();
    } else if (ch[0] == '/' && ch[1] == '/') {
        return ReadLineComment();
    } else if (ch[0] == '/' && ch[1] == '*') {
        if (ch[2] == '*') {
            return ReadDocumentation();
        } else {
            return ReadBlockComment();
        }
    } else if (ch[0] == '_' || (std::isalpha(ch[0]) || (unsigned char)ch[0] >= 0xC0)) {
        return ReadIdentifier();
    } else if (ch[0] == '+' || ch[0] == '-' || 
        ch[0] == '*' || ch[0] == '/' ||
        ch[0] == '%' || ch[0] == '^' ||
        ch[0] == '&' || ch[0] == '|' ||
        ch[0] == '<' || ch[0] == '>' ||
        ch[0] == '=' || ch[0] == '!') {
        return ReadOperator();
    } else if (ch[0] == ',') {
        m_source_stream.Next();
        m_source_location.GetColumn()++;
        return Token(Token_comma, ",", location);
    } else if (ch[0] == ';') {
        m_source_stream.Next();
        m_source_location.GetColumn()++;
        return Token(Token_semicolon, ";", location);
    } else if (ch[0] == ':') {
        m_source_stream.Next();
        m_source_location.GetColumn()++;
        return Token(Token_colon, ":", location);
    } else if (ch[0] == '.') {
        if (ch[1] == '.' && ch[2] == '.') {
            for (int i = 0; i < 3; i++) {
                m_source_stream.Next();
                m_source_location.GetColumn()++;
            }
            return Token(Token_ellipsis, "...", location);
        } else {
            m_source_stream.Next();
            m_source_location.GetColumn()++;
            return Token(Token_dot, ".", location);
        }
    } else if (ch[0] == '(') {
        m_source_stream.Next();
        m_source_location.GetColumn()++;
        return Token(Token_open_parenthesis, "(", location);
    } else if (ch[0] == ')') {
        m_source_stream.Next();
        m_source_location.GetColumn()++;
        return Token(Token_close_parenthesis, ")", location);
    } else if (ch[0] == '[') {
        m_source_stream.Next();
        m_source_location.GetColumn()++;
        return Token(Token_open_bracket, "[", location);
    } else if (ch[0] == ']') {
        m_source_stream.Next();
        m_source_location.GetColumn()++;
        return Token(Token_close_bracket, "]", location);
    } else if (ch[0] == '{') {
        m_source_stream.Next();
        m_source_location.GetColumn()++;
        return Token(Token_open_brace, "{", location);
    } else if (ch[0] == '}') {
        m_source_stream.Next();
        m_source_location.GetColumn()++;
        return Token(Token_close_brace, "}", location);
    } else {
        CompilerError error(Level_fatal, 
            Msg_unexpected_token, location, m_source_stream.Next());

        m_compilation_unit->GetErrorList().AddError(error);
        m_source_location.GetColumn()++;

        return Token(Token_empty, "", location);
    }
}

char Lexer::ReadEscapeCode()
{
    // location of the start of the escape code
    SourceLocation location(m_source_location);

    char esc = '\0';

    if (HasNext()) {
        esc = m_source_stream.Next();
        m_source_location.GetColumn()++;

        // TODO: add support for unicode escapes
        switch (esc) {
        case 't':
            return '\t';
        case 'b':
            return '\b';
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 'f':
            return '\f';
        case '\'':
        case '\"':
        case '\\':
            // return the escape itself
            break;
        default:
            m_compilation_unit->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_unrecognized_escape_sequence, 
                    location, std::string("\\") + esc));
        }
    }

    return esc;
}

Token Lexer::ReadStringLiteral()
{
    // the location for the start of the string
    SourceLocation location(m_source_location);

    std::string value;

    char delim = m_source_stream.Next();
    m_source_location.GetColumn()++;

    char ch = m_source_stream.Peek();
    while (true) {
        // the character as a utf-32 character
        unsigned int ic = 0;
        char *ic_bytes = reinterpret_cast<char*>(&ic);
        // check to see if it is a utf-8 character
        unsigned char uc = (unsigned char)ch;
        if (uc >= 0 && uc <= 127) {
            // 1-byte character
            ic_bytes[0] = m_source_stream.Next();
            m_source_location.GetColumn()++;
        } else if ((uc & 0xE0) == 0xC0) {
            // 2-byte character
            ic_bytes[0] = m_source_stream.Next();
            ic_bytes[1] = m_source_stream.Next();
            m_source_location.GetColumn() += 2;
        } else if ((uc & 0xF0) == 0xE0) {
            // 3-byte character
            ic_bytes[0] = m_source_stream.Next();
            ic_bytes[1] = m_source_stream.Next();
            ic_bytes[2] = m_source_stream.Next();
            m_source_location.GetColumn() += 3;
        } else if ((uc & 0xF8) == 0xF0) {
            // 4-byte character
            ic_bytes[0] = m_source_stream.Next();
            ic_bytes[1] = m_source_stream.Next();
            ic_bytes[2] = m_source_stream.Next();
            ic_bytes[3] = m_source_stream.Next();
            m_source_location.GetColumn() += 4;
        }

        if (ic == (unsigned int)('\n') || !HasNext()) {
            // unterminated string literal
            m_compilation_unit->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_unterminated_string_literal, 
                    location));

            break;
        } else if (ic == (unsigned int)delim) {
            // end of string

            break;
        }

        // determine whether to read an escape sequence
        if (ic == (unsigned int)('\\')) {
            value += ReadEscapeCode();
        } else {
            // Append the character array itself
            value.append(ic_bytes);
        }
        
        ch = m_source_stream.Peek();
    }

    return Token(Token_string_literal, value, location);
}

Token Lexer::ReadNumberLiteral()
{
    SourceLocation location(m_source_location);

    // store the value in a string
    std::string value;

    // assume integer to start
    TokenType type = Token_integer_literal;

    // allows support for floats starting with '.'
    if (m_source_stream.Peek() == '.') {
        type = Token_float_literal;
        value = "0.";
        m_source_stream.Next();
        m_source_location.GetColumn()++;
    }

    char ch = m_source_stream.Peek();
    while (m_source_stream.HasNext() && std::isdigit(ch)) {
        value += m_source_stream.Next();
        m_source_location.GetColumn()++;

        if (type != Token_float_literal) {
            if (m_source_stream.HasNext() && m_source_stream.Peek() == '.') {
                // read float literal
                type = Token_float_literal;
                value += m_source_stream.Next();
                m_source_location.GetColumn()++;
            }
        }

        ch = m_source_stream.Peek();
    }

    return Token(type, value, location);
}

Token Lexer::ReadHexNumberLiteral()
{
    // location of the start of the hex number
    SourceLocation location(m_source_location);

    // store the value in a string
    std::string value;

    // read the "0x"
    for (int i = 0; i < 2; i++) {
        value += m_source_stream.Next();
        m_source_location.GetColumn()++;
    }

    char ch = '\0';
    do {
        value += m_source_stream.Next();
        ch = m_source_stream.Peek();
    } while (std::isxdigit(ch));

    long num = std::strtol(value.c_str(), 0, 16);
    std::stringstream ss;
    ss << num;

    return Token(Token_integer_literal, ss.str(), location);
}

Token Lexer::ReadLineComment()
{
    SourceLocation location(m_source_location);

    // read '//'
    for (int i = 0; i < 2; i++) {
        m_source_stream.Next();
        m_source_location.GetColumn()++;
    }

    // read until newline or EOF is reached
    while (m_source_stream.HasNext() && m_source_stream.Peek() != '\n') {
        m_source_stream.Next();
        m_source_location.GetColumn()++;
    }

    return Token(Token_empty, "", location);
}

Token Lexer::ReadBlockComment()
{
    SourceLocation location(m_source_location);

    // read '/*'
    for (int i = 0; i < 2; i++) {
        m_source_stream.Next();
        m_source_location.GetColumn()++;
    }

    char previous = '\0';
    while (HasNext()) {
        if (m_source_stream.Peek() == '/' && previous == '*') {
            m_source_stream.Next();
            m_source_location.GetColumn()++;
            break;
        }
        previous = m_source_stream.Next();
        m_source_location.GetColumn()++;
    }

    return Token(Token_empty, "", location);
}

Token Lexer::ReadDocumentation()
{
    SourceLocation location(m_source_location);

    std::string value;

    // read '/**'
    for (int i = 0; i < 3; i++) {
        m_source_stream.Next();
        m_source_location.GetColumn()++;
    }

    char previous = '\0';
    while (HasNext()) {
        if (m_source_stream.Peek() == '/' && previous == '*') {
            m_source_stream.Next();
            m_source_location.GetColumn()++;
            break;
        }
        previous = m_source_stream.Next();
        m_source_location.GetColumn()++;
    }

    return Token(Token_documentation, value, location);
}

Token Lexer::ReadOperator()
{
    // location of the start of the hex number
    SourceLocation location(m_source_location);

    std::array<char, 2> ch; 
    for (int i = 0; i < 2; i++) {
        ch[i] = m_source_stream.Peek(i);
    }

    std::string op_2 = { ch[0], ch[1] };
    std::string op_1 = { ch[0] };
    if (Operator::IsOperator(op_2)) {
        m_source_stream.Next();
        m_source_stream.Next();
        m_source_location.GetColumn() += 2;
        return Token(Token_operator, op_2, location);
    } else if (Operator::IsOperator(op_1)) {
        m_source_stream.Next();
        m_source_location.GetColumn()++;
        return Token(Token_operator, op_1, location);
    } else {
        return Token(Token_empty, "", location);
    }
}

Token Lexer::ReadIdentifier()
{
    SourceLocation location(m_source_location);

    // store the name in this string
    std::string value;

    char ch = m_source_stream.Peek();
    while (std::isdigit(ch) || ch == '_' || (std::isalpha(ch) || (unsigned char)ch >= 0xC0)) {
        // the character as a utf-32 character
        unsigned int ic = 0;
        char *ic_bytes = reinterpret_cast<char*>(&ic);
        // check to see if it is a utf-8 character
        unsigned char uc = (unsigned char)ch;
        if (uc >= 0 && uc <= 127) {
            // 1-byte character
            ic_bytes[0] = m_source_stream.Next();
            m_source_location.GetColumn()++;
        } else if ((uc & 0xE0) == 0xC0) {
            // 2-byte character
            ic_bytes[0] = m_source_stream.Next();
            ic_bytes[1] = m_source_stream.Next();
            m_source_location.GetColumn() += 2;
        } else if ((uc & 0xF0) == 0xE0) {
            // 3-byte character
            ic_bytes[0] = m_source_stream.Next();
            ic_bytes[1] = m_source_stream.Next();
            ic_bytes[2] = m_source_stream.Next();
            m_source_location.GetColumn() += 3;
        } else if ((uc & 0xF8) == 0xF0) {
            // 4-byte character
            ic_bytes[0] = m_source_stream.Next();
            ic_bytes[1] = m_source_stream.Next();
            ic_bytes[2] = m_source_stream.Next();
            ic_bytes[3] = m_source_stream.Next();
            m_source_location.GetColumn() += 4;
        }

        // append the character pointer
        value.append(ic_bytes);
        // set ch to be the next character in the buffer
        ch = m_source_stream.Peek();
    } 

    TokenType type = Token_identifier;

    if (Keyword::IsKeyword(value)) {
        type = Token_keyword;
    }
    
    return Token(type, value, location);
}

bool Lexer::HasNext()
{
    if (!m_source_stream.HasNext()) {
        m_compilation_unit->GetErrorList().AddError(
            CompilerError(Level_fatal, Msg_unexpected_eof, m_source_location));
        return false;
    }
    return true;
}

void Lexer::SkipWhitespace()
{
    while (m_source_stream.HasNext() && std::isspace(m_source_stream.Peek())) {
        if (m_source_stream.Next() == '\n') {
            m_source_location.GetLine()++;
            m_source_location.GetColumn() = 0;
        } else {
            m_source_location.GetColumn()++;
        }
    }
}