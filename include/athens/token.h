#ifndef TOKEN_H
#define TOKEN_H

#include <athens/source_location.h>

struct Token {
    Token(const SourceLocation &location);
    Token(const Token &other);

private:
    SourceLocation m_location;
};

#endif