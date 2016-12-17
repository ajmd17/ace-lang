#include <ace-c/lexer.hpp>
#include <ace-c/operator.hpp>
#include <ace-c/compiler_error.hpp>
#include <ace-c/keywords.hpp>

#include <array>
#include <sstream>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>

using namespace utf;

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
    // skip initial whitespace
    SkipWhitespace();

    while (m_source_stream.HasNext() && m_source_stream.Peek() != '\0') {
        Token token = NextToken();
        if (!token.Empty()) {
            m_token_stream->Push(token);
        }

        // SkipWhitespace() returns true if there was a newline
        const SourceLocation location = m_source_location;
        if (SkipWhitespace()) {
            // add the `newline` statement terminator if not a continuation token
            if (token && token.GetType() != Token::TokenType::Token_newline && !token.IsContinuationToken()) {
                // skip whitespace before next token
                SkipWhitespace();

                // check if next token is connected
                if (m_source_stream.HasNext() && m_source_stream.Peek() != '\0') {
                    auto peek = m_source_stream.Peek();
                    if (peek == '{' || peek == '.') {
                        // do not add newline
                        continue;
                    }
                }

                // add newline
                m_token_stream->Push(Token(Token::TokenType::Token_newline, "newline", location));
            }
        }
    }
}

Token Lexer::NextToken()
{
    SourceLocation location = m_source_location;

    std::array<u32char, 3> ch;
    int total_pos_change = 0;
    for (int i = 0; i < 3; i++) {
        int pos_change = 0;
        ch[i] = m_source_stream.Next(pos_change);
        total_pos_change += pos_change;
    }
    // go back to previous position
    m_source_stream.GoBack(total_pos_change);

    if (ch[0] == '\"' || ch[0] == '\'') {
        return ReadStringLiteral();
    } else if (ch[0] == '0' && (ch[1] == 'x' || ch[1] == 'X')) {
        return ReadHexNumberLiteral();
    } else if (utf32_isdigit(ch[0]) || (ch[0] == '.' && utf32_isdigit(ch[1]))) {
        return ReadNumberLiteral();
    } else if (ch[0] == '/' && ch[1] == '/') {
        return ReadLineComment();
    } else if (ch[0] == '/' && ch[1] == '*') {
        return ReadBlockComment();
        /*if (ch[2] == '*') {
            return ReadDocumentation();
        } else {
            return ReadBlockComment();
        }*/
    } else if (ch[0] == '_' || utf32_isalpha(ch[0])) {
        return ReadIdentifier();
    } else if (ch[0] == '-' && ch[1] == '>') {
        for (int i = 0; i < 2; i++) {
            int pos_change = 0;
            m_source_stream.Next(pos_change);
            m_source_location.GetColumn() += pos_change;
        }
        return Token(Token::TokenType::Token_right_arrow, "->", location);
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
        return Token(Token::TokenType::Token_comma, ",", location);
    } else if (ch[0] == ';') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token::TokenType::Token_semicolon, ";", location);
    } else if (ch[0] == ':') {
        if (ch[1] == ':') {
            for (int i = 0; i < 2; i++) {
                int pos_change = 0;
                m_source_stream.Next(pos_change);
                m_source_location.GetColumn() += pos_change;
            }
            return Token(Token::TokenType::Token_double_colon, "::", location);
        } else {
            int pos_change = 0;
            m_source_stream.Next(pos_change);
            m_source_location.GetColumn() += pos_change;
            return Token(Token::TokenType::Token_colon, ":", location);
        }
    } else if (ch[0] == '.') {
        if (ch[1] == '.' && ch[2] == '.') {
            for (int i = 0; i < 3; i++) {
                int pos_change = 0;
                m_source_stream.Next(pos_change);
                m_source_location.GetColumn() += pos_change;
            }
            return Token(Token::TokenType::Token_ellipsis, "...", location);
        } else {
            int pos_change = 0;
            m_source_stream.Next(pos_change);
            m_source_location.GetColumn() += pos_change;
            return Token(Token::TokenType::Token_dot, ".", location);
        }
    } else if (ch[0] == '(') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token::TokenType::Token_open_parenthesis, "(", location);
    } else if (ch[0] == ')') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token::TokenType::Token_close_parenthesis, ")", location);
    } else if (ch[0] == '[') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token::TokenType::Token_open_bracket, "[", location);
    } else if (ch[0] == ']') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token::TokenType::Token_close_bracket, "]", location);
    } else if (ch[0] == '{') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token::TokenType::Token_open_brace, "{", location);
    } else if (ch[0] == '}') {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token::TokenType::Token_close_brace, "}", location);
    } else {
        int pos_change = 0;
        CompilerError error(Level_fatal, Msg_unexpected_token,
            location, m_source_stream.Next(pos_change));

        m_compilation_unit->GetErrorList().AddError(error);
        m_source_location.GetColumn() += pos_change;

        return Token(Token::TokenType::Token_empty, "", location);
    }
}

u32char Lexer::ReadEscapeCode()
{
    // location of the start of the escape code
    SourceLocation location = m_source_location;

    u32char esc = 0;

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
                    location, std::string("\\") + utf::get_bytes(esc)));
            esc = (u32char)'\0';
        }
    }

    return esc;
}

Token Lexer::ReadStringLiteral()
{
    // the location for the start of the string
    SourceLocation location = m_source_location;

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
            value.append(utf::get_bytes(esc));
        } else {
            // Append the character itself
            value.append(utf::get_bytes(ch));
        }

        ch = m_source_stream.Next(pos_change);
    }

    return Token(Token::TokenType::Token_string_literal, value, location);
}

Token Lexer::ReadNumberLiteral()
{
    SourceLocation location = m_source_location;

    // store the value in a string
    std::string value;

    // assume integer to start
    Token::TokenType type = Token::TokenType::Token_integer_literal;

    // allows support for floats starting with '.'
    if (m_source_stream.Peek() == '.') {
        type = Token::TokenType::Token_float_literal;
        value = "0.";
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
    }

    u32char ch = m_source_stream.Peek();
    while (m_source_stream.HasNext() && utf32_isdigit(ch)) {
        int pos_change = 0;
        u32char next_ch = m_source_stream.Next(pos_change);
        value.append(utf::get_bytes(next_ch));
        m_source_location.GetColumn() += pos_change;

        if (type != Token::TokenType::Token_float_literal) {
            if (m_source_stream.HasNext()) {
                // the character as a utf-32 character
                u32char ch = m_source_stream.Peek();
                if (ch == (u32char)'.') {
                    // read next to check if after is a digit
                    int pos_change = 0;
                    m_source_stream.Next(pos_change);
                    
                    u32char next = m_source_stream.Peek();
                    if (!utf::utf32_isalpha(next) && next != (u32char)'_') {
                        // type is a float because of '.' and not an identifier after
                        type = Token::TokenType::Token_float_literal;
                        value.append(utf::get_bytes(ch));
                        m_source_location.GetColumn() += pos_change;
                    } else {
                        // not a float literal, so go back on the '.'
                        m_source_stream.GoBack(pos_change);
                    }
                }
            }
        }
        ch = m_source_stream.Peek();
    }

    return Token(type, value, location);
}

Token Lexer::ReadHexNumberLiteral()
{
    // location of the start of the hex number
    SourceLocation location = m_source_location;

    // store the value in a string
    std::string value;

    // read the "0x"
    for (int i = 0; i < 2; i++) {
        int pos_change = 0;
        u32char next_ch = m_source_stream.Next(pos_change);
        value.append(utf::get_bytes(next_ch));
        m_source_location.GetColumn() += pos_change;
    }

    u32char ch = (u32char)('\0');
    do {
        int pos_change = 0;
        u32char next_ch = m_source_stream.Next(pos_change);
        value.append(utf::get_bytes(next_ch));
        m_source_location.GetColumn() += pos_change;
        ch = m_source_stream.Peek();
    } while (std::isxdigit(ch));

    long num = std::strtol(value.c_str(), 0, 16);
    std::stringstream ss;
    ss << num;

    return Token(Token::TokenType::Token_integer_literal, ss.str(), location);
}

Token Lexer::ReadLineComment()
{
    SourceLocation location = m_source_location;

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

    return Token(Token::TokenType::Token_empty, "", location);
}

Token Lexer::ReadBlockComment()
{
    SourceLocation location = m_source_location;

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

    return Token(Token::TokenType::Token_empty, "", location);
}

Token Lexer::ReadDocumentation()
{
    SourceLocation location = m_source_location;

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
        } else {
            char ch[4] = {'\0'};
            utf::char32to8(m_source_stream.Peek(), ch);
            // append value
            value += ch;
            
            if (m_source_stream.Peek() == (u32char)'\n') {
                // just reset column and increment line
                m_source_location.GetColumn() = 0;
                m_source_location.GetLine()++;
            }
        }
        int pos_change = 0;
        previous = m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
    }

    return Token(Token::TokenType::Token_documentation, value, location);
}

Token Lexer::ReadOperator()
{
    // location of the start of the hex number
    SourceLocation location = m_source_location;

    std::array<u32char, 2> ch;
    int total_pos_change = 0;
    for (int i = 0; i < 2; i++) {
        int pos_change = 0;
        ch[i] = m_source_stream.Next(pos_change);
        total_pos_change += pos_change;
    }
    // go back
    m_source_stream.GoBack(total_pos_change);

    std::string op_1 = utf::get_bytes(ch[0]);
    std::string op_2 = op_1 + utf::get_bytes(ch[1]);

    if (Operator::IsOperator(op_2)) {
        int pos_change_1 = 0, pos_change_2 = 0;
        m_source_stream.Next(pos_change_1);
        m_source_stream.Next(pos_change_2);
        m_source_location.GetColumn() += (pos_change_1 + pos_change_2);
        return Token(Token::TokenType::Token_operator, op_2, location);
    } else if (Operator::IsOperator(op_1)) {
        int pos_change = 0;
        m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        return Token(Token::TokenType::Token_operator, op_1, location);
    } else {
        return Token(Token::TokenType::Token_empty, "", location);
    }
}

Token Lexer::ReadIdentifier()
{
    SourceLocation location = m_source_location;

    // store the name in this string
    std::string value;

    // the character as a utf-32 character
    u32char ch = m_source_stream.Peek();

    while (utf32_isdigit(ch) || ch == (u32char)('_') || utf32_isalpha(ch)) {
        int pos_change = 0;
        ch = m_source_stream.Next(pos_change);
        m_source_location.GetColumn() += pos_change;
        // append the raw bytes
        value.append(utf::get_bytes(ch));
        // set ch to be the next character in the buffer
        ch = m_source_stream.Peek();
    }

    Token::TokenType type = Token::TokenType::Token_identifier;

    if (Keyword::IsKeyword(value)) {
        type = Token::TokenType::Token_keyword;
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

bool Lexer::SkipWhitespace()
{
    bool had_newline = false;

    while (m_source_stream.HasNext() && utf32_isspace(m_source_stream.Peek())) {
        int pos_change = 0;
        if (m_source_stream.Next(pos_change) == (u32char)'\n') {
            m_source_location.GetLine()++;
            m_source_location.GetColumn() = 0;
            had_newline = true;
        } else {
            m_source_location.GetColumn() += pos_change;
        }
    }

    return had_newline;
}
