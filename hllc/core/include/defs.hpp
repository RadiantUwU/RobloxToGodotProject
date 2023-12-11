#ifndef HLLC_DEFS
#define HLLC_DEFS

#ifdef USE_GODOT
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/error_macros.hpp>
#ifdef DEBUG_ENABLED
#define HLLC_DEBUG
#endif
#else // from godot_cpp/core/defs.hpp and error macros
#ifdef __GNUC__
#define FUNCTION_STR __FUNCTION__
#else
#define FUNCTION_STR __FUNCTION__
#endif
#ifdef _MSC_VER
#define GENERATE_TRAP() __debugbreak()
#else
#define GENERATE_TRAP() __builtin_trap()
#endif
#if defined(__GNUC__)
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) x
#define unlikely(x) x
#ifndef NDEBUG
#define HLLC_DEBUG
#endif
#endif
#endif

#ifndef HLLC_DEBUG
#define HLLC_INLINE inline
#else
#define HLLC_INLINE
#endif

#endif