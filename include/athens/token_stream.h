#ifndef TOKEN_STREAM_H
#define TOKEN_STREAM_H

#include <athens/token.h>

#include <vector>

class TokenStream {
public:
    TokenStream();
    TokenStream(const TokenStream &other) = delete;

    void Push(const Token &token);
    const Token &Last() const { return m_tokens.back(); }
    const Token &Next();
    inline bool HasNext() const { return m_position < m_tokens.size(); }
    inline const Token *Peek(int n = 0) const 
    {
        size_t pos = m_position + n;
        if (pos >= m_tokens.size()) {
            return nullptr;
        }
        return &m_tokens[pos]; 
    }

    std::vector<Token> m_tokens;
    size_t m_position;
};

#endif