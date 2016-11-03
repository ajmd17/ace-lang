#ifndef ERROR_LIST_HPP
#define ERROR_LIST_HPP

#include <ace-c/compiler_error.hpp>

#include <vector>
#include <algorithm>

class ErrorList {
public:
    ErrorList();
    ErrorList(const ErrorList &other);

    inline void AddError(const CompilerError &error) { m_errors.push_back(error); }
    inline void ClearErrors() { m_errors.clear(); }
    inline void SortErrors() { std::sort(m_errors.begin(), m_errors.end()); }

    bool HasFatalErrors() const;

    std::vector<CompilerError> m_errors;
};

#endif
