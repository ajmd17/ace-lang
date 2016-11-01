#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP

#include <string>

class Exception {
public:
    Exception(const std::string &str);
    Exception(const Exception &other);

    inline const std::string &ToString() const { return m_str; }

private:
    std::string m_str;
};

#endif
