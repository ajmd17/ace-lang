#ifndef TYPSDEFS_HPP
#define TYPEDEFS_HPP

#include <cstdint>

#define MED_PRECISION 1

#if LOW_PRECISION

typedef int16_t a_int;
typedef float a_float;

#elif MED_PRECISION

typedef int32_t a_int;
typedef float a_float;

#elif HIGH_PRECISION

typedef int64_t a_int;
typedef double a_float;

#endif

#endif
