#include <lua.h>
#include <functional>
#include "string.hpp"

#ifndef HLLC_MISC
#define HLLC_MISC

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
    bool unique_ptr = false;
    size_t type_hash;
#ifndef NDEBUG
    const char* type_name;
#endif
    std::function<void()> destructor;
union {
    LuaObjectWrapper obj;
    double number;
    String str;
    void* ptr;
    lua_CFunction cfunc;
};
public:
    LuaVariantWrapper(const LuaObjectWrapper o);
    LuaVariantWrapper(const LuaVariantWrapper& v);
    LuaVariantWrapper(const double n) : number(n), type(TYPE_NUMBER) {}
    LuaVariantWrapper(const char* s) : str(s), type(TYPE_STRING) {}
    LuaVariantWrapper(const char* s, size_t l) : str(s,l), type(TYPE_STRING) {}
    LuaVariantWrapper(const String s) : str(s), type(TYPE_STRING) {}
    LuaVariantWrapper(std::nullptr_t) : type(TYPE_NIL) {}
    LuaVariantWrapper(lua_CFunction cf) : cfunc(cf), type(TYPE_CFUNC) {}
    template<typename T>
    LuaVariantWrapper(T* o);
    template<typename T>
    LuaVariantWrapper(const T* o);
    template<typename T>
    LuaVariantWrapper(const T o);
    ~LuaVariantWrapper();

    template<typename T>
    bool is_ud_type() const;
#ifndef NDEBUG
    const char* get_ud_typename() const;
#endif

    template <typename T>
    const T* cast_ud_const() const;
    template <typename T>
    T* cast_ud() const;
    template <typename T>
    inline T cast();
    template<>
    inline double cast<>() {return number;}
    template<>
    inline const char* cast<>() {return str.s;}
    template<>
    inline String cast<>() {return str;}
    template<>
    inline LuaObjectWrapper cast<>() {return obj;}
    template<>
    inline std::nullptr_t cast<>() {return nullptr;}
    template<>
    inline lua_CFunction cast<>() {return cfunc;}
};



}

#endif