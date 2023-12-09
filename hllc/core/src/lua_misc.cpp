#include <hllc/core/include/lua_misc.hpp>
#include <functional>
#include <lua.h>
#include <type_traits>
#include "string.hpp"

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

}