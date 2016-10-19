#include <athens/error_list.hpp>

#include <algorithm>

ErrorList::ErrorList()
{
}

ErrorList::ErrorList(const ErrorList &other)
    : m_errors(other.m_errors)
{
}

bool ErrorList::HasFatalErrors() const
{
    if (!m_errors.empty()) {
        for (const CompilerError &error : m_errors) {
            if (error.GetLevel() == Level_fatal) {
                return true;
            }
        }
    }
    return false;
}
