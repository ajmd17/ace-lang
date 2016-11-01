#include <ace-vm/exception.hpp>

Exception::Exception(const std::string &str)
    : m_str(str)
{
}

Exception::Exception(const Exception &other)
    : m_str(other.m_str)
{
}
