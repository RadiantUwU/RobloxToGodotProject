#include <lua.h>
#include <lualib.h>
#include "rblx_main.hpp"
#include "rblx_instance.hpp"

namespace godot {

luau_State::luau_State(RobloxVMInstance *VM) {
    vm = VM;
    L = vm->create_lua_state();
    ::lua_pushlightuserdata(L, VM);
    ::lua_rawsetfield(L, LUA_REGISTRYINDEX, "ROBLOX_VM");
    ::lua_pushlightuserdata(L, this);
    ::lua_rawsetfield(L, LUA_REGISTRYINDEX, "LUAU_STATE");
}

LuaObject::LuaObject(luau_State *L) {
    luau_context state = L;
    ref = state.new_ref(-1);
    ls = L;
    // stack is auto resized by luau_context
}
LuaObject::LuaObject(luau_State *L, int idx) {
    luau_context state = L;
    ref = state.new_ref(idx);
    ls = L;
    // stack is auto resized by luau_context
}
LuaObject::~LuaObject() {
    luau_context origin = ls;
    origin.delete_ref(ref);
}
void LuaObject::get(luau_State *to) {
    luau_context origin = ls;
    luau_context to_ = to;
    to_.dont_clear_stack();
    origin.push_ref(ref);
    switch (origin.get_type(-1)) {
        case LUA_TTHREAD:
        case LUA_TNIL:
            to_.push_object(); // nil
            break;
        case LUA_TBOOLEAN:
        case LUA_TNUMBER:
        case LUA_TSTRING:
        case LUA_TLIGHTUSERDATA:
            to_.push_object(origin.to_object());
            break;
        case LUA_TFUNCTION:
            if (origin.is_cfunction(-1)) {
                to_.push_object(origin.as_cfunc());
            } else {
                to_.push_object(); // nil
            }
            break;
        case LUA_TUSERDATA:
            //TODO: make a proxy object
            to_.push_object();
            break;
        case LUA_TTABLE:
            to_.new_table();
            origin.get(-1,1);
            if (!origin.is_type(-1, LUA_TNIL)) {
                origin.pop_stack(1);
                int key = 0;
                while (true) {
                    luau_context iter = ls;
                    key = iter.rawiter(-1, key);
                    if (key == -1) break;
                    if (!iter.is_type(-2, LUA_TSTRING)) continue; // automatic popping of the key and value
                }
                
            }
    }
}

}