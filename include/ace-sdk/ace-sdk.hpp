#ifndef ACE_SDK_HPP
#define ACE_SDK_HPP

#ifndef __cplusplus
#error Ace SDK requires a C++ compiler
#endif

#ifdef _WIN32
#define ACE_EXPORT extern "C" __declspec(dllexport)
#else
#define ACE_EXPORT extern "C"
#endif

#define ACE_FUNCTION(name) ACE_EXPORT void name(ace::sdk::Params params)

// forward declarations
struct VMState;
struct ExecutionThread;
struct StackValue;

namespace ace {
namespace sdk {

struct Params {
    VMState* state;
    ExecutionThread* thread;
    StackValue** args;
    int nargs;
};

} // namespace sdk
} // namespace ace

#endif
