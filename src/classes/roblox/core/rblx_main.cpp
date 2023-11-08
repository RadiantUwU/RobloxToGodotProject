#include <lua.h>
#include <lualib.h>
#include "rblx_main.hpp"
#include "rblx_instance.hpp"
#include "rblx_events.hpp"
#include "rblx_debug.hpp"

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
LuaObject::LuaObject(const LuaObject& o) {
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
void LuaObject::get(luau_State *to) const {
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
bool LuaObject::operator==(const LuaObject& o) const {
    luau_context origin = ls;
    origin.push_ref(ref);
    origin.push_object(o);
    return origin.rawequal(-2, -1);
}
bool LuaObject::operator!=(const LuaObject& o) const {
    return not (*this == o);
}

void luau_context::push_object(const RBXVariant& v) {
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
void luau_context::push_object(const RBXVariant& v, int idx) {
    push_object(v);
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
void luau_context::push_object(Instance *p) {
    if (p == nullptr) {
        push_object();
    } else {
        getregistry("USERDATA_REFS");
        rawget(-1,(size_t)(void*)p);
        remove_stack(-2);
    }
}
void luau_context::push_object(Instance *p, int idx) {
    push_object(p);
    ::lua_insert(L, idx);
}
void luau_context::push_object(RBXScriptSignal *p) {
    if (p == nullptr) {
        push_object();
    } else {
        getregistry("USERDATA_REFS");
        rawget(-1,(size_t)(void*)p);
        remove_stack(-2);
    }
}
void luau_context::push_object(RBXScriptSignal *p, int idx) {
    push_object(p);
    ::lua_insert(L, idx);
}
TaskScheduler* luau_context::get_task_scheduler() {
    return get_vm()->task;
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
    ctx.rawset(LUA_REGISTRYINDEX,"USERDATA_REFS");
    ctx.newmetatable_type(ctx.UD_TINSTANCE);
    ctx.set_dtor(ctx.UD_TINSTANCE, Instance::delete_instance);
    ctx.push_object(&Instance::lua_static_get,"Instance::__index");
    ctx.rawset(-2, "__index");
    ctx.push_object(&Instance::lua_static_set,"Instance::__newindex");
    ctx.rawset(-2, "__newindex");
    ctx.push_object(&Instance::lua_static_tostring,"Instance::__tostring");
    ctx.rawset(-2, "__tostring");
    ctx.push_object("Instance");
    ctx.rawset(-2, "__type");
    ctx.pop_stack(1);
}
void RobloxVMInstance::register_genv(lua_State *L) {
    luau_context ctx = L;

    ctx.new_table();
    ctx.push_object(&Instance::new_instance,"Instance::new");
    ctx.rawset(-2,"new");
    ctx.freeze(-1);
    ctx.setglobal("Instance");

    ctx.push_object(&RobloxVMInstance::lua_getmetatable_override,"<protected getmetatable>");
    ctx.setglobal("getmetatable");
    ctx.push_object(&RobloxVMInstance::lua_setmetatable_override,"<protected setmetatable>");
    ctx.setglobal("setmetatable");
    
    ctx.push_object(&RobloxVMInstance::lua_typeof,"typeof");
    ctx.setglobal("typeof");
}
void RobloxVMInstance::register_registry(lua_State *L) {
    luau_context ctx = L;
    ctx.new_table();
    ctx.setregistry("INSTANCE_TAGS");

    ctx.new_table();
    ctx.push_object("k");
    ctx.rawset(-2,"__mode");
    ctx.setregistry("WEAKTABLE_K");
    ctx.new_table();
    ctx.push_object("V");
    ctx.rawset(-2,"__mode");
    ctx.setregistry("WEAKTABLE_V");
}
RobloxVMInstance::RobloxVMInstance(lua_State *main) {
    main_synchronized = new(memalloc(sizeof(luau_State))) luau_State(this, main);;
    task = new(memalloc(sizeof(TaskScheduler))) TaskScheduler(this);
    new(main_synchronized) luau_State(this, main);
    register_types(main);
    register_genv(main);
    register_registry(main);
}
RobloxVMInstance::~RobloxVMInstance() {
    task->~TaskScheduler();
    memfree(task);
    memfree(main_synchronized);
}

TaskScheduler::TaskScheduler(RobloxVMInstance *VM) {
    this->vm = VM;
    luau_context ctx = VM->main_synchronized;
    ctx.new_table();
    ctx.push_object(TaskScheduler::lua_task_spawn,"task::spawn");
    ctx.rawset(-2,"spawn");
    ctx.push_object(TaskScheduler::lua_task_defer,"task::defer");
    ctx.rawset(-2,"defer");
    ctx.push_object(TaskScheduler::lua_task_delay,"task::delay");
    ctx.rawset(-2,"delay");
    ctx.push_object(TaskScheduler::lua_task_synchronize,"task::synchronize");
    ctx.rawset(-2,"synchronize");
    ctx.push_object(TaskScheduler::lua_task_desynchronize,"task::desynchronize");
    ctx.rawset(-2,"desynchronize");
    ctx.push_object(TaskScheduler::lua_task_wait,"task::wait");
    ctx.rawset(-2,"wait");
    ctx.push_object(TaskScheduler::lua_task_cancel,"task::cancel");
    ctx.rawset(-2,"cancel");
    ctx.freeze(-1);
    ctx.setglobal("task");

    ctx.new_table();
    ctx.setregistry("TASK_legacy_spawn");
    ctx.new_table();
    ctx.setregistry("TASK_legacy_delay");
    ctx.new_table();
    ctx.setregistry("TASK_legacy_wait");
    ctx.new_table();
    ctx.setregistry("TASK_await_defer");
    ctx.new_table();
    ctx.setregistry("TASK_await_delay");
    ctx.new_table();
    ctx.setregistry("TASK_await_wait");
}
TaskScheduler::~TaskScheduler() {}
int TaskScheduler::lua_task_spawn(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_type_argument(1,"functionOrThread", LUA_TFUNCTION, LUA_TTHREAD);
    fn.assert_stack_size_vararg(1);
    bool isthread = false;
    if (fn.is_type(1, LUA_TFUNCTION)) {
        fn.new_thread(fn.get_stack_size() - 1);
    } else {
        isthread = true;
        fn.push_value(1);
        int status = fn.costatus();
        if (status != LUA_COSUS) {
            fn.error("cannot spawn a non-suspended thread (invalid argument #1)");
        }
    }
    fn.push_value(1,1);
    fn.resume((isthread) ? fn.get_stack_size()-2 : 0, 0, 0); // TODO: Implement error handle function
    return 1;
}
int TaskScheduler::lua_task_defer(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_type_argument(1,"functionOrThread", LUA_TFUNCTION, LUA_TTHREAD);
    fn.assert_stack_size_vararg(1);
    int nargs = fn.get_stack_size();
    bool isthread = false;
    if (fn.is_type(1, LUA_TFUNCTION)) {
        fn.push_value(1);
        fn.new_thread(0);
        fn.remove_stack(1);
        fn.insert_into(1); // replace function with thread
        fn.push_value(1,1);
    } else {
        isthread = true;
        fn.push_value(1);
        int status = fn.costatus();
        if (status != LUA_COSUS) {
            fn.error("cannot spawn a non-suspended thread (invalid argument #1)");
        }
    }
    fn.create_array_from_stack(nargs);
    fn.getregistry("TASK_await_defer");
    fn.push_value(2);
    fn.rawset(-2,fn.len(-2)+1);
    return 1;
}
int TaskScheduler::lua_task_delay(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_type_argument(1,"functionOrThread", LUA_TFUNCTION, LUA_TTHREAD);
    fn.assert_type_argument(2,"delay", LUA_TNUMBER);
    fn.assert_stack_size_vararg(2);
    int nargs = fn.get_stack_size();
    bool isthread = false;
    if (fn.is_type(1, LUA_TFUNCTION)) {
        fn.push_value(1);
        fn.new_thread(0);
        fn.remove_stack(1);
        fn.insert_into(1); // replace function with thread
        fn.push_value(1,1);
    } else {
        isthread = true;
        fn.push_value(1);
        int status = fn.costatus();
        if (status != LUA_COSUS) {
            fn.error("cannot spawn a non-suspended thread (invalid argument #1)");
        }
    }
    fn.create_array_from_stack(nargs);
    fn.getregistry("TASK_await_delay");
    fn.push_value(2);
    fn.rawset(-2,fn.len(-2)+1);
    return 1;
}
int TaskScheduler::lua_task_synchronize(lua_State *L) {
    return 0; // TODO: Implement desync/sync mode.
}
int TaskScheduler::lua_task_desynchronize(lua_State *L) {
    return 0; // TODO: Implement desync/sync mode.
}
int TaskScheduler::lua_task_cancel(lua_State *L) {
    return 0; // TODO: Implement task::cancel
}

}