#include <lua_misc.hpp>
#include <functional>
#include <lua.h>
#include <memory>
#include <typeinfo>
#include <type_traits>
#include "string.hpp"
#include "hash_map.hpp"

namespace HLLC::_LLC {

LuaObjectWrapper::LuaObjectWrapper(lua_State *L) {
    ref = lua_ref(L, -1);
    oL = L;
}
LuaObjectWrapper::LuaObjectWrapper(const LuaObjectWrapper& o) {
    oL = o.oL;
    lua_getref(oL, o.ref);
    ref = lua_ref(oL, -1);
}
LuaObjectWrapper::~LuaObjectWrapper() {
    lua_unref(oL, ref);
}

LuaVariantWrapper::LuaVariantWrapper(const LuaVariantWrapper& o) {
    type = o.type;
    is_const = o.is_const;
    is_destructible = o.is_destructible;
    has_initializer = o.has_initializer;
    has_copy_constructor = o.has_copy_constructor;
    unique_ptr = o.unique_ptr;
    ptr_size = o.ptr_size;
    initializer = o.initializer;
    destructor = o.destructor;
    if (unique_ptr) {
        ptr = memalloc(ptr_size);
        if (has_copy_constructor) {
            copy_constructor(ptr,o.ptr);
        } else if (has_initializer) {
            memcpy(ptr,o.ptr,ptr_size);
            initializer(ptr);
        } else {
            memcpy(ptr,o.ptr,ptr_size);
        }
    } else {
        ptr = o.ptr;
    }
}
template <typename T>
LuaVariantWrapper::LuaVariantWrapper(T* o) {
    type_hash = typeid(T).hash_code();
#ifndef NDEBUG
    type_name = typeid(T).name();
#endif
    ptr = o;
    type = LuaVariantWrapper::TYPE_LIGHTUSERDATA;
}
template <typename T>
LuaVariantWrapper::LuaVariantWrapper(const T* o) {
    is_const = true;
    type_hash = typeid(T).hash_code();
#ifndef NDEBUG
    type_name = typeid(T).name();
#endif
    ptr = *(void**)&o;
    type = LuaVariantWrapper::TYPE_LIGHTUSERDATA;
}
template <typename T>
LuaVariantWrapper::LuaVariantWrapper(T o) {
    unique_ptr = true;
    ptr_size = sizeof(T);
    type_hash = typeid(T).hash_code();
#ifndef NDEBUG
    type_name = typeid(T).name();
#endif
    if (std::is_default_constructible<T>::value) {
        initializer = [](void* p) {new (p) T();};
        has_initializer = true;
    }
    if (std::is_copy_constructible<T>::value) {
        copy_constructor = [](void* p, void* po) {new (p) T(*(T*)po);};
        has_copy_constructor = true;
    }
    if (std::is_destructible<T>::value) {
        destructor = [](void* p) {std::destroy_at<T>(p);};
        is_destructible = true;
    }
    ptr = memalloc(ptr_size);
    if (has_copy_constructor) {
        copy_constructor(ptr,&o);
    } else if (has_initializer) {
        memcpy(ptr,o.ptr,ptr_size);
        initializer(ptr);
    } else {
        memcpy(ptr,o.ptr,ptr_size);
    }
    type = LuaVariantWrapper::TYPE_PRIMITIVE_USERDATA;
}
LuaVariantWrapper::~LuaVariantWrapper() {
    switch (type) {
    case TYPE_STRING:
        str.~String();
        return;
    case TYPE_LUA_OBJECT:
        obj.~LuaObjectWrapper();
        return;
    case TYPE_PRIMITIVE_USERDATA:
        if (is_destructible) destructor(ptr);
        return;
    default:
        return;
    }
}
template <typename T>
bool LuaVariantWrapper::is_ud_type() const {
    return type_hash == typeid(T).hash_code();
}
#ifndef NDEBUG
const char* LuaVariantWrapper::get_ud_typename() const {
    return type_name;
}
#endif
template <typename T>
const T* LuaVariantWrapper::cast_ud_const() const {
    assert(is_ud_type<T>());
    return (const T*)ptr;
}
template <typename T>
T* LuaVariantWrapper::cast_ud() const {
    assert(is_ud_type<T>());
    assert(!is_const);
    return (T*)ptr;
}
template <typename T>
T LuaVariantWrapper::cast() const {
#ifndef NDEBUG
    assert(is_ud_type<T>());
#endif
    return *(T*)ptr;
}


}