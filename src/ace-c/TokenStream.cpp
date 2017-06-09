#include <ace-c/TokenStream.hpp>

TokenStream::TokenStream(const TokenStreamInfo &info)
    : m_position(0),
      m_info(info)
{
}
