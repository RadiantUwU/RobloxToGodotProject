#ifndef RBLX_DEBUG
#define RBLX_DEBUG

#ifndef NDEBUG
//#define RBLX_PRINT_STACK_CLEARS //Prints whenever the luau_context clears the stack
#endif

#ifndef NDEBUG
#define RBLX_INLINE
#include <iostream>
namespace godot {
namespace roblox_debug_private {
    template <typename T>
    void RBLX_PRINT_VERBOSE_(T arg) {
        ::std::cout << arg;
    }
    template <typename T, typename... Args>
    void RBLX_PRINT_VERBOSE_(T o,Args... args) {
        ::std::cout << o;
        RBLX_PRINT_VERBOSE_(args...);
    }
}
template <typename... Args>
void RBLX_PRINT_VERBOSE(Args... args) {
    roblox_debug_private::RBLX_PRINT_VERBOSE_(args...);
    ::std::cout << "\n";
}
}
#ifdef __GNUC__
#define RBLX_NOINLINE [[gnu::noinline]]
#else
#ifdef _MSC_VER
#define RBLX_NOINLINE __declspec(noinline)
#else
#ifdef __clang__
#define RBLX_NOINLINE [[clang::noinline]]
#endif
#endif
#endif
#else
// Dont even make it a function, let the compiler completely skip over it
#define RBLX_PRINT_VERBOSE(...)
#define RBLX_INLINE inline
#define RBLX_NOINLINE
#endif

#endif