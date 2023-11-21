#include <lua.h>
#include <lualib.h>
#include <luacode.h>
#include <cstdlib>
#include "rblx_main.hpp"
#include "rblx_instance.hpp"
#include "rblx_events.hpp"
#include "rblx_debug.hpp"

namespace godot {

static lua_CompileOptions rblx_vm_compile_options = {
    // int optimizationLevel;
    1,

    // int debugLevel;
    1,

    // int coverageLevel;
    0,

    // const char* vectorLib;
    nullptr,

    // const char* vectorCtor;
    nullptr,

    // const char** mutableGlobals;
};

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
int luau_context::compile(const char* fname, LuaString code, int env_idx) {
    size_t bytecode_size;
    char* bytecode = luau_compile(code.s, code.l, &rblx_vm_compile_options, &bytecode_size);
    if (bytecode == nullptr) {
        return -1;
    }
    int status = luau_load(L, fname, bytecode, bytecode_size, env_idx);
    std::free(bytecode);
    return status;
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

    ctx.new_table();
    ctx.push_object(&TaskScheduler::lua_task_spawn, "task::spawn");
    ctx.rawset(-2, "spawn");
    ctx.push_object(&TaskScheduler::lua_task_defer, "task::defer");
    ctx.rawset(-2, "defer");
    ctx.push_object(&TaskScheduler::lua_task_delay, "task::delay");
    ctx.rawset(-2, "delay");
    ctx.push_object(&TaskScheduler::lua_task_synchronize, "task::synchronize");
    ctx.rawset(-2, "synchronize");
    ctx.push_object(&TaskScheduler::lua_task_desynchronize, "task::desynchronize");
    ctx.rawset(-2, "desynchronize");
    ctx.push_object(&TaskScheduler::lua_task_wait, "task::wait");
    ctx.rawset(-2, "wait");
    ctx.push_object(&TaskScheduler::lua_task_cancel, "task::cancel");
    ctx.rawset(-2, "cancel");
    ctx.freeze(-1);
    ctx.setglobal("task");
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
#ifndef NDEBUG
    context = RUNCTXT_CORE; // Allow core access when in debug mode
#endif
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
    fn.resume((isthread) ? fn.get_stack_size()-2 : 0, 0, 0); // TODO: Implement error handler function
    return 1;
}
int TaskScheduler::lua_task_defer(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_type_argument(1,"functionOrThread", LUA_TFUNCTION, LUA_TTHREAD); // ok
    fn.assert_stack_size_vararg(1);
    int nargs = fn.get_stack_size();
    bool isthread = false;
#ifndef NDEBUG
    RBLX_PRINT_VERBOSE("task::defer new");
    fn.print_stack();
#endif
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
    fn.pop_stack(2);
    return 1;
}
int TaskScheduler::lua_task_delay(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_type_argument(1,"functionOrThread", LUA_TFUNCTION, LUA_TTHREAD);
    fn.assert_type_argument(2,"delay", LUA_TNUMBER);
    fn.assert_stack_size_vararg(2);
#ifndef NDEBUG
    RBLX_PRINT_VERBOSE("task::delay");
    fn.print_stack();
#endif
    double delay = (double)fn.as_object(2);
    if (delay < 0) delay = 0;
    fn.push_object(delay + fn.clock(), 3);
    RBLX_PRINT_VERBOSE("lua_task_delay: on stack pos 2 is_number = ",(fn.as_object(2).type == RBXVariant::Type::RBXVARIANT_NUM) ? "true" : "false");
    RBLX_PRINT_VERBOSE("lua_task_delay: on stack pos 3 is_number = ",(fn.as_object(3).type == RBXVariant::Type::RBXVARIANT_NUM) ? "true" : "false");
    int nargs = fn.get_stack_size();
    bool isthread = false;
    if (fn.is_type(1, LUA_TFUNCTION)) {
        fn.push_value(1);
        fn.new_thread(0);
        fn.remove_stack(1);
        fn.insert_into(1); // replace function with thread
    } else {
        isthread = true;
        fn.push_value(1);
        int status = fn.costatus();
        if (status != LUA_COSUS) {
            fn.error("cannot spawn a non-suspended thread (invalid argument #1)");
        }
        fn.pop_stack(1);
    }
    fn.push_value(1, 1); // copy thread into position 1 to return it
    fn.create_array_from_stack(nargs);
    fn.getregistry("TASK_await_delay");
    fn.push_value(2); // pushes the new array
    fn.rawset(-2,fn.len(-2)+1);
    fn.pop_stack(2);
    return 1; // returns thread
}
int TaskScheduler::lua_task_synchronize(lua_State *L) {
    return 0; // TODO: Implement desync/sync mode.
}
int TaskScheduler::lua_task_desynchronize(lua_State *L) {
    return 0; // TODO: Implement desync/sync mode.
}
int TaskScheduler::lua_task_cancel(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(1);
    fn.assert_type_argument(1, "thread", LUA_TTHREAD);
    fn.push_value(1);
    int status = fn.costatus();
    if (status == LUA_COSUS) {
        lua_resetthread(fn.as_thread(1));
    }
    return 0;
}
int TaskScheduler::lua_task_wait(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(0,1);
#ifndef NDEBUG
    RBLX_PRINT_VERBOSE("task::delay");
    fn.print_stack();
#endif
    double delay = 0;
    if (fn.get_stack_size() == 1) {
        fn.assert_type_argument(1,"delay", LUA_TNUMBER);
        delay = (double)fn.to_object(1);
    }
    fn.push_current_thread();
    fn.push_object(delay);
    fn.push_object(delay+fn.clock());
    fn.create_array_from_stack(3);
    fn.getregistry("TASK_await_wait");
    fn.push_value(1);
    fn.rawset(-2,fn.len(-2)+1);
    fn.yield(0);
    return 0;
}
bool TaskScheduler::resume_cycle(lua_State *L) {
    luau_context ctx = L;
    int64_t iter;
    // TODO: Implement deprecated stuff
    // {thread, funcargs...}
    ctx.getregistry("TASK_await_defer");
    ctx.clone_table();
    ctx.setregistry("TASK_await_defer"); // put new table in it
    iter = 0;
    while (true) {
        luau_context for_loop_scope = L;
        iter = ctx.rawiter(-1,iter);
        if (iter == -1) break;
        int nargs = ctx.push_vararg_array_and_pop(-1);
        RBLX_PRINT_VERBOSE("task::resume defer iter=",iter," nargs=",nargs);
        ctx.push_value(-nargs);
        if (ctx.costatus() == LUA_COSUS) ctx.resume(nargs-1,0,0);
    }
    ctx.pop_stack(1);
    RBLX_PRINT_VERBOSE("task::resume defer done stack=",ctx.get_stack_size());

    // {thread, delay, time+delay, funcargs...}
    ctx.getregistry("TASK_await_delay");
    ctx.clone_table();
    ctx.setregistry("TASK_await_delay"); // put new table in it
    iter = 0;
    while (true) {
        luau_context for_loop_scope = L;
        iter = ctx.rawiter(-1,iter);
        if (iter == -1) break;
        ctx.rawget(-1, 3);
        double time = ctx.to_object();
        RBLX_PRINT_VERBOSE("task::resume delay iter=",iter," start_in=",time-ctx.clock());
        if (time > ctx.clock()) {
            ctx.rawget(-1, 2);
            double time_to_wait = ctx.to_object();
            int nargs = ctx.push_vararg_array_and_pop(-1);
            ctx.remove_stack(-nargs+2); // remove time
            ctx.remove_stack(-nargs+2); // remove time_to_wait
            ctx.push_value(-(nargs-2));
            if (ctx.costatus() == LUA_COSUS) ctx.resume(nargs-3,0,0); // NOTE: I thought you had to supply how much it had to wait, nope!
        } else {
            ctx.getregistry("TASK_await_delay");
            ctx.push_value(-3);
            ctx.push_value(-3);
            ctx.rawset(-3);
        }
    }
    ctx.pop_stack(1);
    RBLX_PRINT_VERBOSE("task::resume delay done stack=",ctx.get_stack_size());
    
    // {thread, delay, time+delay}
    ctx.getregistry("TASK_await_wait");
    ctx.clone_table();
    ctx.setregistry("TASK_await_wait"); // put new table in it
    iter = 0;
    while (true) {
        luau_context for_loop_scope = L;
        iter = ctx.rawiter(-1,iter);
        if (iter == -1) break;
        ctx.rawget(-1, 3);
        double time = ctx.to_object();
        RBLX_PRINT_VERBOSE("task::resume wait iter=",iter," resume_in=",time-ctx.clock());
        if (time > ctx.clock()) {
            ctx.rawget(-1, 2);
            double time_to_wait = ctx.to_object();
            ctx.push_vararg_array_and_pop(-1);
            ctx.pop_stack(2);
            ctx.push_value(-1);
            if (ctx.costatus() == LUA_COSUS) {
                ctx.push_object(ctx.clock() - time + time_to_wait);
                ctx.resume(1, 0, 0); // NOTE: I thought you had to supply how much it had to wait, nope!
            }
        } else {
            ctx.getregistry("TASK_await_wait");
            ctx.push_value(-3);
            ctx.push_value(-3);
            ctx.rawset(-3);
        }
        //int nargs = ctx.push_vararg_array_and_pop(-1);
    }
    ctx.pop_stack(1);
    RBLX_PRINT_VERBOSE("task::resume wait done stack=",ctx.get_stack_size());

    ctx.getregistry("TASK_await_defer");
    if (ctx.len(-1) > 0) return true;
    return false;
}
bool TaskScheduler::resume_cycle(luau_State *L) {
    luau_context ctx = L;
    int64_t iter;
    // TODO: Implement deprecated stuff
    // {thread, funcargs...}
    ctx.getregistry("TASK_await_defer");
    ctx.new_table();
    ctx.setregistry("TASK_await_defer"); // put new table in it
    iter = 0;
    while (true) {
        luau_context for_loop_scope = L;
        iter = ctx.rawiter(-1,iter);
        if (iter == -1) break;
        int nargs = ctx.push_vararg_array_and_pop(-1);
        RBLX_PRINT_VERBOSE("task::resume defer iter=",iter," nargs=",nargs);
#ifndef NDEBUG
        RBLX_PRINT_VERBOSE("task::resume on defer");
        for_loop_scope.print_stack();
#endif
        ctx.push_value(-nargs);
        if (ctx.costatus() == LUA_COSUS) ctx.resume(nargs-1,0,0);
    }
    ctx.pop_stack(1);
    RBLX_PRINT_VERBOSE("task::resume defer done stack=",ctx.get_stack_size());

    // {thread, delay, time+delay, funcargs...}
    ctx.getregistry("TASK_await_delay");
    ctx.new_table();
    ctx.setregistry("TASK_await_delay"); // put new table in it
    iter = 0;
    while (true) {
        luau_context for_loop_scope = L;
        iter = ctx.rawiter(-1,iter);
        if (iter == -1) break;
        ctx.rawget(-1, 3);
        double time = ctx.to_object();
        RBLX_PRINT_VERBOSE("task::resume delay iter=",iter," start_in=",time-ctx.clock());
        if (time > ctx.clock()) {
            ctx.rawget(-1, 2);
            double time_to_wait = ctx.to_object();
            int nargs = ctx.push_vararg_array_and_pop(-1);
            ctx.remove_stack(-nargs+2); // remove time
            ctx.remove_stack(-nargs+2); // remove time_to_wait
            ctx.push_value(-(nargs-2));
            if (ctx.costatus() == LUA_COSUS) ctx.resume(nargs-3,0,0); // NOTE: I thought you had to supply how much it had to wait, nope!
        } else {
            ctx.getregistry("TASK_await_delay");
            ctx.push_value(-3);
            ctx.push_value(-3);
            ctx.rawset(-3);
        }
    }
    ctx.pop_stack(1);
    RBLX_PRINT_VERBOSE("task::resume delay done stack=",ctx.get_stack_size());
    
    // {thread, delay, time+delay}
    ctx.getregistry("TASK_await_wait");
    ctx.new_table();
    ctx.setregistry("TASK_await_wait"); // put new table in it
    iter = 0;
    while (true) {
        luau_context for_loop_scope = L;
        iter = ctx.rawiter(-1,iter);
        if (iter == -1) break;
        ctx.rawget(-1, 3);
        double time = ctx.to_object();
        RBLX_PRINT_VERBOSE("task::resume wait iter=",iter," resume_in=",time-ctx.clock());
        if (time > ctx.clock()) {
            ctx.rawget(-1, 2);
            double time_to_wait = ctx.to_object();
            ctx.push_vararg_array_and_pop(-1);
            ctx.pop_stack(2);
            ctx.push_value(-1);
            if (ctx.costatus() == LUA_COSUS) {
                ctx.push_object(ctx.clock() - time + time_to_wait);
                ctx.resume(1, 0, 0); // NOTE: I thought you had to supply how much it had to wait, nope!
            }
        } else {
            ctx.getregistry("TASK_await_wait");
            ctx.push_value(-3);
            ctx.push_value(-3);
            ctx.rawset(-3);
        }
        //int nargs = ctx.push_vararg_array_and_pop(-1);
    }
    ctx.pop_stack(1);
    RBLX_PRINT_VERBOSE("task::resume wait done stack=",ctx.get_stack_size());

    ctx.getregistry("TASK_await_defer");
    if (ctx.len(-1) > 0) return true;
    return false;
}

}