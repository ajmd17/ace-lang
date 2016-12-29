#ifndef TOKEN_STREAM_HPP
#define TOKEN_STREAM_HPP

#include <ace-c/Token.hpp>
#include <common/my_assert.hpp>

#include <vector>

class TokenStream {
public:
    TokenStream();
    TokenStream(const TokenStream &other) = delete;
    
    inline Token Peek(int n = 0) const
    {
        size_t pos = m_position + n;
        if (pos >= m_tokens.size()) {
            return Token::EMPTY;
        }
        return m_tokens[pos];
    }

    inline void Push(const Token &token) { m_tokens.push_back(token); }
    inline bool HasNext() const { return m_position < m_tokens.size(); }
    inline Token Next() { ASSERT(m_position < m_tokens.size()); return m_tokens[m_position++]; }
    inline Token Last() const { ASSERT(!m_tokens.empty()); return m_tokens.back(); }
    inline size_t GetSize() const { return m_tokens.size(); }
    inline size_t GetPosition() const { return m_position; }
    inline bool Eof() const { return m_position >= m_tokens.size(); }

    std::vector<Token> m_tokens;
    size_t m_position;
};

#endif
