#ifndef RBLX_DEBUG
#define RBLX_DEBUG
//#define VERBOSE_RBLX
#ifdef VERBOSE_RBLX
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
#else
// Dont even make it a function, let the compiler completely skip over it
#define RBLX_PRINT_VERBOSE(...)
#endif

#endif