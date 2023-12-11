#include <lua.h>
#include <functional>
#include "string.hpp"
#include "defs.hpp"

#ifndef HLLC_MISC
#define HLLC_MISC

#ifdef USE_GODOT
#include <godot_cpp/core/error_macros.hpp>
#ifdef DEBUG_ENABLED
#define assert(m_cond)                                                                                                           \
	if (unlikely(!(m_cond))) {                                                                                                   \
		::godot::_err_print_error(FUNCTION_STR, __FILE__, __LINE__, "FATAL: assertion failed  \"" _STR(m_cond) "\" is false.");  \
		::godot::_err_flush_stdout();                                                                                            \
		GENERATE_TRAP();                                                                                                         \
	} else                                                                                                                       \
		((void)0)
#define assert_msg(m_cond, m_msg)                                                                                                       \
	if (unlikely(!(m_cond))) {                                                                                                          \
		::godot::_err_print_error(FUNCTION_STR, __FILE__, __LINE__, "FATAL: assertion failed  \"" _STR(m_cond) "\" is false.", m_msg);  \
		::godot::_err_flush_stdout();                                                                                                   \
		GENERATE_TRAP();                                                                                                                \
	} else                                                                                                                              \
		((void)0)
#else
#define assert(m_cond)
#endif
#else
#include <iostream>
#ifndef DEBUG_ENABLED
#define assert(m_cond)                                                                                                                               \
	if (unlikely(!(m_cond))) {                                                                                                                       \
		::std::cerr << FUNCTION_STR << ' ' << __FILE__ << ':' << __LINE__ << " FATAL: assertion failed  \"" _STR(m_cond) "\" is false." << ::std::endl; \
		GENERATE_TRAP();                                                                                                                             \
	} else                                                                                                                                           \
		((void)0)
#define assert_msg(m_cond, m_msg)                                                                                                                                     \
	if (unlikely(!(m_cond))) {                                                                                                                                 \
		::std::cerr << FUNCTION_STR << ' ' << __FILE__ << ':' << __LINE__ << " FATAL: assertion failed  \"" _STR(m_cond) "\" is false. " << m_msg << ::std::endl; \
		GENERATE_TRAP();                                                                                                                                       \
	} else                                                                                                                                                     \
		((void)0)
#else
#define assert(m_cond)
#define assert_msg(m_cond,m_msg)
#endif
#endif

namespace HLLC::_LLC {

class LuaObjectWrapper {
    size_t ref;
    lua_State *oL;
public:
    inline size_t get_ref() const {return ref;}
    LuaObjectWrapper(lua_State *L);
    LuaObjectWrapper(const LuaObjectWrapper& o);
    ~LuaObjectWrapper();
};

class LuaVariantWrapper {
public:
    enum Type {
        TYPE_NIL,
        TYPE_LUA_OBJECT,
        TYPE_NUMBER,
        TYPE_STRING,
        TYPE_LIGHTUSERDATA,
        TYPE_PRIMITIVE_USERDATA,
        TYPE_CFUNC
    };
    Type get_type() const {
        return type;
    }
private:
    Type type;
    bool is_const = false;
    bool is_destructible = false;
    bool has_initializer = false;
    bool has_copy_constructor = false;
    bool unique_ptr = false;
    size_t ptr_size = 0;
    size_t type_hash;
#ifndef NDEBUG
    const char* type_name;
#endif
    std::function<void(void*,void*)> copy_constructor;
    std::function<void(void*)> initializer;
    std::function<void(void*)> destructor;
union {
    LuaObjectWrapper obj;
    double number;
    String str;
    void* ptr;
    lua_CFunction cfunc;
};
public:
    LuaVariantWrapper(const LuaObjectWrapper o) : obj(o), type(TYPE_LUA_OBJECT) {};
    LuaVariantWrapper(const LuaVariantWrapper& v);
    LuaVariantWrapper(const double n) : number(n), type(TYPE_NUMBER) {}
    LuaVariantWrapper(const char* s) : str(s), type(TYPE_STRING) {}
    LuaVariantWrapper(const char* s, size_t l) : str(s,l), type(TYPE_STRING) {}
    LuaVariantWrapper(const String s) : str(s), type(TYPE_STRING) {}
    LuaVariantWrapper(std::nullptr_t) : type(TYPE_NIL) {}
    LuaVariantWrapper() : type(TYPE_NIL) {}
    LuaVariantWrapper(lua_CFunction cf) : cfunc(cf), type(TYPE_CFUNC) {}
    template<typename T>
    LuaVariantWrapper(T* o);
    template<typename T>
    LuaVariantWrapper(const T* o);
    template<typename T>
    LuaVariantWrapper(T o);
    ~LuaVariantWrapper();

    template<typename T>
    bool is_ud_type() const;
    size_t get_type_hash() const {return type_hash;}
#ifndef NDEBUG
    const char* get_ud_typename() const;
#endif

    template <typename T>
    const T* cast_ud_const() const;
    template <typename T>
    T* cast_ud() const;
    template <typename T>
    inline T cast() const;
    template<>
    inline double cast<>() const {return number;}
    template<>
    inline const char* cast<>() const {return str.s;}
    template<>
    inline String cast<>() const {return str;}
    template<>
    inline LuaObjectWrapper cast<>() const {return obj;}
    template<>
    inline std::nullptr_t cast<>() const {return nullptr;}
    template<>
    inline lua_CFunction cast<>() const {return cfunc;}
};

} // namespace HLLC::_LLC

#endif