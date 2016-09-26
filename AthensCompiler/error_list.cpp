#include "error_list.h"

#include <algorithm>

ErrorList::ErrorList()
{
}

ErrorList::ErrorList(const ErrorList &other)
    : m_errors(other.m_errors)
{
}

void ErrorList::AddError(const CompilerError &error)
{
    m_errors.push_back(error);
}

void ErrorList::SortErrors()
{
    std::sort(m_errors.begin(), m_errors.end());
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