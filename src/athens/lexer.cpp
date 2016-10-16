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

    std::array<u32char, 3> ch;
    int total_pos_change = 0;
    for (int i = 0; i < 3; i++) {
        int pos_change = 0;
        ch[i] = m_source_stream.Next(pos_change);
        total_pos_change += pos_change;
    }
    // go back to previous position
    m_source_stream.GoBack(total_pos_change);

    if (ch[0] == '\"') {
        return ReadStringLiteral();
    } else if (utf32_isdigit(ch[0]) || ch[0] == '.') {
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
    } else if (ch[0] == '_' || utf32_isalpha(ch[0])) {
        return ReadIdentifier();
    } else if (ch[0] == '+' || ch[0] == '-' || 
        ch[0] == '*' || ch[0] == '/' ||
        ch[0] == '%' || ch[0] == '^' ||
        ch[0] == '&' || ch[0] == '|' ||
        ch[0] == '<' || ch[0] == '>' ||
        ch[0] == '=' || ch[0] == '!') {
        return ReadOperator();
    } else if (ch[0] == ',') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token_comma, ",", location);
    } else if (ch[0] == ';') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token_semicolon, ";", location);
    } else if (ch[0] == ':') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token_colon, ":", location);
    } else if (ch[0] == '.') {
        if (ch[1] == '.' && ch[2] == '.') {
            for (int i = 0; i < 3; i++) {
                int pos_change = 0;
                m_source_stream.Next(pos_change);
                m_source_location.GetColumn() += pos_change;
            }
            return Token(Token_ellipsis, "...", location);
        } else {
            int pos_change = 0;
            m_source_stream.Next(pos_change);
            m_source_location.GetColumn() += pos_change;
            return Token(Token_dot, ".", location);
        }
    } else if (ch[0] == '(') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token_open_parenthesis, "(", location);
    } else if (ch[0] == ')') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token_close_parenthesis, ")", location);
    } else if (ch[0] == '[') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token_open_bracket, "[", location);
    } else if (ch[0] == ']') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token_close_bracket, "]", location);
    } else if (ch[0] == '{') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token_open_brace, "{", location);
    } else if (ch[0] == '}') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token_close_brace, "}", location);
    } else {
        int pos_change = 0;
        CompilerError error(Level_fatal, 
            Msg_unexpected_token, location, m_source_stream.Next(pos_change));

        m_compilation_unit->GetErrorList().AddError(error);
        m_source_location.GetColumn() += pos_change;

        return Token(Token_empty, "", location);
    }
}

u32char Lexer::ReadEscapeCode()
{
    // location of the start of the escape code
    SourceLocation location(m_source_location);

    u32char esc;

    if (HasNext()) {
        int pos_change = 0;
        esc = m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;

        // TODO: add support for unicode escapes
        switch (esc) {
        case 't':
            esc = (u32char)'\t';
        case 'b':
            esc = (u32char)'\b';
        case 'n':
            esc = (u32char)'\n';
        case 'r':
            esc = (u32char)'\r';
        case 'f':
            esc = (u32char)'\f';
        case '\'':
        case '\"':
        case '\\':
            // return the escape itself
            break;
        default:
            m_compilation_unit->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_unrecognized_escape_sequence, 
                    location, std::string("\\") + utf32_get_bytes(esc)));
            esc = (u32char)'\0';
        }
    }

    return esc;
}

Token Lexer::ReadStringLiteral()
{
    // the location for the start of the string
    SourceLocation location(m_source_location);

    std::string value;

    int pos_change = 0;

    u32char delim = m_source_stream.Next(pos_change);
    m_source_location.GetColumn() += pos_change;

    // the character as utf-32
    u32char ch = m_source_stream.Next(pos_change);

    while (true) {
        m_source_location.GetColumn() += pos_change;

        if (ch == (u32char)'\n' || !HasNext()) {
            // unterminated string literal
            m_compilation_unit->GetErrorList().AddError(
                CompilerError(Level_fatal, Msg_unterminated_string_literal, 
                    location));

            if (ch == (u32char)'\n') {
                // increment line and reset column
                m_source_location.GetColumn() = 0;
                m_source_location.GetLine()++;
            }

            break;
        } else if (ch == delim) {
            // end of string
            break;
        }

        // determine whether to read an escape sequence
        if (ch == (u32char)'\\') {
            u32char esc = ReadEscapeCode();
            // append the bytes
            value.append(utf32_get_bytes(esc));
        } else {
            // Append the character itself
            value.append(utf32_get_bytes(ch));
        }
        
        ch = m_source_stream.Next(pos_change);
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
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
    }

    u32char ch = m_source_stream.Peek();
    while (m_source_stream.HasNext() && utf32_isdigit(ch)) {
        int pos_change = 0;
        u32char next_ch = m_source_stream.Next(pos_change);
        value.append(utf32_get_bytes(next_ch));
        m_source_location.GetColumn() += pos_change;

        if (type != Token_float_literal) {
            if (m_source_stream.HasNext() && m_source_stream.Peek() == '.') {
                // read float literal
                type = Token_float_literal;
                int pos_change = 0;
                u32char next_ch = m_source_stream.Next(pos_change);
                value.append(utf32_get_bytes(next_ch));
                m_source_location.GetColumn() += pos_change;
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
        int pos_change = 0;
        u32char next_ch = m_source_stream.Next(pos_change);
        value.append(utf32_get_bytes(next_ch));
        m_source_location.GetColumn() += pos_change;
    }

    u32char ch = '\0';
    do {
        int pos_change = 0;
        u32char next_ch = m_source_stream.Next(pos_change);
        value.append(utf32_get_bytes(next_ch));
        m_source_location.GetColumn() += pos_change;
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
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
    }

    // read until newline or EOF is reached
    while (m_source_stream.HasNext() && m_source_stream.Peek() != (u32char)'\n') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
    }

    return Token(Token_empty, "", location);
}

Token Lexer::ReadBlockComment()
{
    SourceLocation location(m_source_location);

    // read '/*'
    for (int i = 0; i < 2; i++) {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
    }

    u32char previous = 0;
    while (HasNext()) {
        if (m_source_stream.Peek() == (u32char)'/' && previous == (u32char)'*') {
            int pos_change = 0;
            m_source_stream.Next(pos_change);
            m_source_location.GetColumn() += pos_change;
            break;
        } else if (m_source_stream.Peek() == (u32char)'\n') {
            // just reset column and increment line
            m_source_location.GetColumn() = 0;
            m_source_location.GetLine()++;
        }
        int pos_change = 0;
        previous = m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
    }

    return Token(Token_empty, "", location);
}

Token Lexer::ReadDocumentation()
{
    SourceLocation location(m_source_location);

    std::string value;

    // read '/**'
    for (int i = 0; i < 3; i++) {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
    }

    u32char previous = 0;
    while (HasNext()) {
        if (m_source_stream.Peek() == (u32char)'/' && previous == (u32char)'*') {
            int pos_change = 0;
            m_source_stream.Next(pos_change);
            m_source_location.GetColumn() += pos_change;
            break;
        }
        int pos_change = 0;
        previous = m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
    }

    return Token(Token_documentation, value, location);
}

Token Lexer::ReadOperator()
{
    // location of the start of the hex number
    SourceLocation location(m_source_location);

    std::array<u32char, 2> ch;
    int total_pos_change = 0;
    for (int i = 0; i < 2; i++) {
        int pos_change = 0;
        ch[i] = m_source_stream.Next(pos_change);
        total_pos_change += pos_change;
    }
    // go back
    m_source_stream.GoBack(total_pos_change);

    std::string op_2;
    op_2 += utf32_get_bytes(ch[0]);
    op_2 += utf32_get_bytes(ch[1]);

    std::string op_1;
    op_1 += utf32_get_bytes(ch[0]);

    if (Operator::IsOperator(op_2)) {
        int pos_change_1 = 0, pos_change_2 = 0;
        m_source_stream.Next(pos_change_1);
        m_source_stream.Next(pos_change_2);
        m_source_location.GetColumn() += (pos_change_1 + pos_change_2);
        return Token(Token_operator, op_2, location);
    } else if (Operator::IsOperator(op_1)) {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
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

    // the character as a utf-32 character
    u32char ch = m_source_stream.Peek();

    while (utf32_isdigit(ch) || ch == (u32char)'_' || utf32_isalpha(ch)) {
        int pos_change = 0;
        ch = m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        // append the raw bytes
        value.append(utf32_get_bytes(ch));
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
    while (m_source_stream.HasNext() && utf32_isspace(m_source_stream.Peek())) {
        int pos_change = 0;
        if (m_source_stream.Next(pos_change) == (u32char)'\n') {
            m_source_location.GetLine()++;
            m_source_location.GetColumn() = 0;
        } else {
            m_source_location.GetColumn() += pos_change;
        }
    }
}