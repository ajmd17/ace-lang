#ifndef MY_ASSERT_HPP
#define MY_ASSERT_HPP

#include <iostream>

#ifndef STRINGIFY_P1
#define STRINGIFY_P1(x) #x
#endif

#ifndef STRINGIFY_P2
#define STRINGIFY_P2(x) STRINGIFY_P1(x)
#endif

#ifdef ASSERT
    #error ASSERT is already defined.
#else
    #define ASSERT(cond) \
        do { \
            if (!(cond)) { \
                std::cerr << ("Assertion `" #cond "` failed in file " STRINGIFY_P2(__FILENAME__) " on line " STRINGIFY_P2(__LINE__) "\n"); \
                std::terminate(); \
            } \
        } while (0)
#endif


#ifdef ASSERT_MSG
    #error ASSERT_MSG is already defined.
#else
    #define ASSERT_MSG(cond, msg) \
        do { \
            if (!(cond)) { \
                std::cerr << ("Assertion `" #cond "` failed in file " STRINGIFY_P2(__FILENAME__) " on line " STRINGIFY_P2(__LINE__) " with message: `") \
                          << (msg) << "`\n"; \
                std::terminate(); \
            } \
        } while (0)
#endif

#endif