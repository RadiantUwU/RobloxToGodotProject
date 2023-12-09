#ifndef HLLC_MEMORY
#define HLLC_MEMORY

#ifdef USE_GODOT
#include <godot_cpp/core/memory.hpp>
#else
#include <cstdlib>
#endif

namespace HLLC {
#ifndef USE_GODOT
    #define memalloc(m_size) malloc(m_size)
    #define memrealloc(m_mem, m_size) realloc(m_mem, m_size)
    #define memfree(m_mem) free(m_mem)
    #define memnew(m_class) new (malloc(typeof(m_class))) m_class
#endif
}

#endif