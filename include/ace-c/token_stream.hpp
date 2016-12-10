#ifndef TOKEN_STREAM_HPP
#define TOKEN_STREAM_HPP

#include <ace-c/token.hpp>
#include <common/my_assert.hpp>

#include <vector>

class TokenStream {
public:
    TokenStream();
    TokenStream(const TokenStream &other) = delete;
    
    inline void Push(const Token &token) { m_tokens.push_back(token); }
    inline Token Last() const
        { ASSERT(!m_tokens.empty()); return m_tokens.back(); }
    inline Token Next()
        { ASSERT(m_position < m_tokens.size()); return m_tokens[m_position++]; }
    inline bool HasNext() const { return m_position < m_tokens.size(); }
    inline Token Peek(int n = 0) const
    {
        size_t pos = m_position + n;
        if (pos >= m_tokens.size()) {
            return Token::EMPTY;
        }
        return m_tokens[pos];
    }

    std::vector<Token> m_tokens;
    size_t m_position;
};

#endif
