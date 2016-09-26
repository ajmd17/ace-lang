#ifndef ERROR_LIST_H
#define ERROR_LIST_H

#include <athens/compiler_error.h>

#include <vector>

class ErrorList {
public:
    ErrorList();
    ErrorList(const ErrorList &other);

    void AddError(const CompilerError &error);
    void SortErrors();
    bool HasFatalErrors() const;

    std::vector<CompilerError> m_errors;
};

#endif