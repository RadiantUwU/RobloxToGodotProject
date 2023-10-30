#include <lua.h>
#include <lualib.h>
#include "rblx_main.hpp"
#include "rblx_instance.hpp"
#include "rblx_events.hpp"

namespace godot {

luau_State::luau_State(RobloxVMInstance *VM) {
    vm = VM;
    L = vm->create_lua_state();
    ::lua_pushlightuserdata(L, this);
    ::lua_rawsetfield(L, LUA_REGISTRYINDEX, "LUAU_STATE");
    ::lua_newtable(L);
    ::lua_rawsetfield(L, LUA_REGISTRYINDEX, "USERDATA_METATABLES");
}
luau_State::~luau_State() {
    ::lua_close(this->L);
}

LuaObject::LuaObject(luau_State *L) {
    luau_context state = L;
    ref = state.new_ref(-1);
    state.pop_stack(1);
    ls = L;
}
LuaObject::LuaObject(luau_State *L, int idx) {
    luau_context state = L;
    ref = state.new_ref(idx);
    state.remove_stack(idx);
    ls = L;
}
LuaObject::LuaObject(LuaObject& o) {
    ls = o.ls;
    luau_context state = ls;
    state.push_ref(o.ref);
    ref = state.new_ref(-1);
    // auto resize stack
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
    switch (origin.get_type(-1)) { // TODO: fix this janky mess
        case LUA_TTHREAD:
        case LUA_TNIL:
            to_.push_object(); // nil
            break;
        case LUA_TBOOLEAN:
            to_.push_object((bool)origin.to_object());
            break;
        case LUA_TNUMBER:
            to_.push_object((double)origin.to_object());
            break;
        case LUA_TSTRING:{
            RBXVariant v = origin.to_object();
            to_.push_object(v.get_str(),(size_t)v.get_slen());
            break;
        }
        case LUA_TLIGHTUSERDATA: {
            RBXVariant v = origin.to_object();
            to_.push_object((void*)v);
            break;
        }
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
            ls->tables_in_conversion.push_back(origin.as_absolute_stack_index(-1));
            origin.get(-1,1);
            if (!origin.is_type(-1, LUA_TNIL)) { // check if it is an array starting from 1 or not
                origin.pop_stack(1);
                int key = 0;
                while (true) {
                    luau_context iter = ls;
                    key = iter.rawiter(-1, key);
                    if (key == -1) break;
                    if (!iter.is_type(-2, LUA_TSTRING)) continue; // automatic popping of the key and value
                    if (1) {
                        RBXVariant _k = origin.to_object(-2);
                        to_.push_object(_k); // pop key leaving only value on stack
                    }
                    switch (origin.get_type(-1)) {
                    case LUA_TNUMBER:
                    case LUA_TBOOLEAN:
                    case LUA_TLIGHTUSERDATA:
                    case LUA_TSTRING:
                    case LUA_TNIL:{ // theoretically impossible because key wouldnt exist but have it as a case anyway
                        RBXVariant _v = origin.to_object(-1);
                        to_.push_object(_v); // let auto do its thing
                        to_.set(-3);
                        break;
                    }
                        
                    case LUA_TTABLE:
                        for (int tbl_idx : to->tables_in_conversion) {
                            if (origin.rawequal(-1,tbl_idx)) {
                                to_.pop_stack(1); // pop the key
                                goto continue_out; // delete luau_context upon reloading loop. causes a stack pop in origin
                            }
                            RBXVariant _v = origin.to_object(-1);
                            to_.push_object(_v);
                            to_.set(-3);
                            break;
                        }
                    case LUA_TUSERDATA:
                        // TODO: create a proxy object 
                        to_.pop_stack(1); // pop the key
                        continue; // delete luau_context upon reloading loop. causes a stack pop in origin
                    case LUA_TFUNCTION:
                        if (iter.is_cfunction(-1)) {
                            to_.push_object(origin.as_cfunc());
                            to_.set(-3);
                        } else {
                            to_.pop_stack(1);
                            continue;
                        }
                        break;
                    }
                    continue_out: ;
                }
            } else {
                origin.pop_stack(1);
                int key = 0;
                int lastkey = 0;
                while (true) {
                    luau_context iter = ls;
                    key = iter.rawiter(-1, key);
                    if (key == -1) break;
                    if (key != lastkey+1) break;
                    if (!iter.is_type(-2, LUA_TNUMBER)) continue; // automatic popping of the key and value
                    {
                        RBXVariant v = origin.to_object(-2);
                        to_.push_object(v); // pop key leaving only value on stack
                    }
                    switch (origin.get_type(-1)) {
                    case LUA_TNUMBER:
                    case LUA_TBOOLEAN:
                    case LUA_TLIGHTUSERDATA:
                    case LUA_TSTRING:
                    case LUA_TNIL:{ // theoretically impossible because key wouldnt exist but have it as a case anyway
                        RBXVariant _v = origin.to_object(-1);
                        to_.push_object(_v); // let auto do its thing
                        to_.set(-3);
                        break;
                    }
                    case LUA_TTABLE:
                        for (int tbl_idx : to->tables_in_conversion) {
                            if (origin.rawequal(-1,tbl_idx)) {
                                to_.pop_stack(1); // pop the key
                                continue; // delete luau_context upon reloading loop. causes a stack pop in origin
                            }
                            RBXVariant v = origin.to_object(-1);
                            to_.push_object(v);
                            to_.set(-3);
                            break;
                        }
                    case LUA_TUSERDATA:
                        // TODO: create a proxy object 
                        to_.pop_stack(1); // pop the key
                        continue; // delete luau_context upon reloading loop. causes a stack pop in origin
                    case LUA_TFUNCTION:
                        if (iter.is_cfunction(-1)) {
                            to_.push_object(origin.as_cfunc());
                            to_.set(-3);
                        } else {
                            to_.pop_stack(1);
                            continue;
                        }
                        break;
                    }
                }
            }
            ls->tables_in_conversion.remove_at(-1);
            break;
    }
    // value is pushed, origin on auto-clear
}

void luau_context::push_object(RBXVariant& v) {
    switch (v.type) {
    case RBXVariant::Type::RBXVARIANT_NIL:
        ::lua_pushnil(L);
        break;
    case RBXVariant::Type::RBXVARIANT_BOOL:
        ::lua_pushboolean(L, (bool)v);
        break;
    case RBXVariant::Type::RBXVARIANT_INT:
        ::lua_pushinteger(L, (int)v);
        break;
    case RBXVariant::Type::RBXVARIANT_NUM:
        ::lua_pushnumber(L, (double)v);
        break;
    case RBXVariant::Type::RBXVARIANT_OBJ:
        push_object((LuaObject&)v);
        break;
    case RBXVariant::Type::RBXVARIANT_STR:
        ::lua_pushlstring(L, v.get_str(), v.get_slen());
        break;
    case RBXVariant::Type::RBXVARIANT_PTR:
        ::lua_pushlightuserdata(L, (void*)v);
        break;
    default:
        break;
    }
}
void luau_context::push_object(RBXVariant& v, int idx) {
    switch (v.type) {
    case RBXVariant::Type::RBXVARIANT_NIL:
        ::lua_pushnil(L);
        break;
    case RBXVariant::Type::RBXVARIANT_BOOL:
        ::lua_pushboolean(L, (bool)v);
        break;
    case RBXVariant::Type::RBXVARIANT_INT:
        ::lua_pushinteger(L, (int)v);
        break;
    case RBXVariant::Type::RBXVARIANT_NUM:
        ::lua_pushnumber(L, (double)v);
        break;
    case RBXVariant::Type::RBXVARIANT_OBJ:
        push_object((LuaObject&)v);
        break;
    case RBXVariant::Type::RBXVARIANT_STR:
        ::lua_pushlstring(L, v.get_str(), v.get_slen());
        break;
    case RBXVariant::Type::RBXVARIANT_PTR:
        ::lua_pushlightuserdata(L, (void*)v);
        break;
    default:
        break;
    }
    ::lua_insert(L, idx);
}
RBXVariant luau_context::as_object() {
    RBXVariant v;
    switch (get_type(-1)) {
    case LUA_TNIL:
        v = RBXVariant();
        break;
    case LUA_TNUMBER:
        v = RBXVariant(::lua_tonumber(L, -1));
        break;
    case LUA_TBOOLEAN:
        v = RBXVariant((bool)(::lua_toboolean(L, -1)));
        break;
    case LUA_TSTRING: {
            const char *s;
            size_t l;
            s = ::lua_tolstring(L, -1, &l);
            v = RBXVariant(s, l);
        }
        break;
    case LUA_TLIGHTUSERDATA:
        v = RBXVariant(::lua_tolightuserdata(L, -1));
        break;
    case LUA_TTABLE:
    case LUA_TUSERDATA:
        ::lua_pushvalue(L, -1);
        LuaObject lo = LuaObject(ls);
        v = RBXVariant(lo);
        break;
    }
    return v;
}
RBXVariant luau_context::as_object(int idx) {
    RBXVariant v;
    switch (get_type(idx)) {
    case LUA_TNIL:
        v = RBXVariant();
        break;
    case LUA_TNUMBER:
        v = RBXVariant(::lua_tonumber(L, idx));
        break;
    case LUA_TBOOLEAN:
        v = RBXVariant((bool)(::lua_toboolean(L, idx)));
        break;
    case LUA_TSTRING: {
            const char *s;
            size_t l;
            s = ::lua_tolstring(L, idx, &l);
            v = RBXVariant(s, l);
        }
        break;
    case LUA_TLIGHTUSERDATA:
        v = RBXVariant(::lua_tolightuserdata(L, idx));
        break;
    case LUA_TTABLE:
    case LUA_TUSERDATA:
        ::lua_pushvalue(L, idx);
        LuaObject lo = LuaObject(ls);
        v = RBXVariant(lo);
        break;
    }
    return v;
}
RBXVariant luau_context::to_object() {
    RBXVariant v = as_object();
    pop_stack(1);
    return v;
}
RBXVariant luau_context::to_object(int idx) {
    RBXVariant v = as_object(idx);
    remove_stack(idx);
    return v;
}

void RobloxVMInstance::register_types(lua_State *L) { // TODO: add __type
    luau_context ctx = L;

    ctx.newmetatable_type(ctx.UD_TRBXSCRIPTSIGNAL);
    ctx.set_dtor(ctx.UD_TRBXSCRIPTSIGNAL, RBXScriptSignal::lua_destroy);
    ctx.push_object(&RBXScriptSignal::lua_get, "RBXScriptSignal::__index");
    ctx.rawset(-2, "__index");
    ctx.pop_stack(1);

    ctx.newmetatable_type(ctx.UD_TRBXSCRIPTCONNECTION);
    ctx.push_object(&RBXScriptConnection::lua_get, "RBXScriptConnection::__index");
    ctx.rawset(-2, "__index");
    ctx.pop_stack(1);

    ctx.new_table();
    ctx.new_table();
    ctx.push_object("v");
    ctx.rawset(-2,"__mode");
    ctx.setmetatable(-2);
    ctx.rawset(LUA_REGISTRYINDEX,"INSTANCE_REFS");
    ctx.newmetatable_type(ctx.UD_TINSTANCE);
    ctx.push_object(&Instance::lua_static_get,"Instance::__index");
    ctx.rawset(-2, "__index");
    ctx.push_object(&Instance::lua_static_set,"Instance::__newindex");
    ctx.rawset(-2, "__newindex");
    ctx.pop_stack(1);
}
void RobloxVMInstance::register_genv(lua_State *L) {
    luau_context ctx = L;
    ctx.push_object();
    ctx.rawset(LUA_GLOBALSINDEX,"vector"); // deyeet

    ctx.new_table();
    ctx.push_object(&Instance::new_instance,"Instance::new");
    ctx.rawset(-2,"new");
    ctx.rawset(LUA_GLOBALSINDEX,"Instance");
}
RobloxVMInstance::RobloxVMInstance(lua_State *main) {
    main_synchronized = new luau_State(this, main);
    register_types(main);
    register_genv(L);
}
RobloxVMInstance::~RobloxVMInstance() {
    delete main_synchronized;
}

}