#include <ace-c/token_stream.hpp>

TokenStream::TokenStream()
	: m_position(0)
{
}

void TokenStream::Push(const Token &token)
{
    m_tokens.push_back(token);
}

const Token &TokenStream::Next()
{
    return m_tokens[m_position++];
}
