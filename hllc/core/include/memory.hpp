#ifndef HLLC_MEMORY
#define HLLC_MEMORY

#ifdef USE_GODOT

#include <godot_cpp/core/memory.hpp>

#else

#include <cstdlib>
#define memalloc(m_size) malloc(m_size)
#define memrealloc(m_mem, m_size) realloc(m_mem, m_size)
#define memfree(m_mem) free(m_mem)
#define memnew(m_class) new (malloc(sizeof(m_class))) m_class

#endif

#endif