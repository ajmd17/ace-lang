#ifndef ACE_TYPEDEFS_HPP
#define ACE_TYPEDEFS_HPP

#include <ace-sdk/ace-sdk.hpp>

#include <stdint.h>

namespace ace {
namespace vm {
  
// forward declarations
struct Value;
struct VMState;
struct ExecutionThread;

// native typedefs
typedef void(*NativeFunctionPtr_t)(ace::sdk::Params);
typedef void(*NativeInitializerPtr_t)(VMState*, ExecutionThread *thread, Value*);

} // namespace vm

typedef int32_t aint32;
typedef int64_t aint64;
typedef float   afloat32;
typedef double  afloat64;

} // namespace ace

#endif