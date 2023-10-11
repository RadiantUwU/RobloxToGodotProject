#include <classes/luau_vm.h>

#include <cstdlib>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/classes/node.hpp>
#include <lua.h>
#include <lualib.h>
#include <utils.h>



void lua_setnode(lua_State* L, godot::LuauVM* node) {
    lua_pushstring(L, GDLUAU_REGISTRY_NODE_KEY);
    lua_pushlightuserdata(L, node);
    lua_settable(L, LUA_REGISTRYINDEX);
}

godot::LuauVM* lua_getnode(lua_State* L) {
    lua_pushstring(L, GDLUAU_REGISTRY_NODE_KEY);
    lua_gettable(L, LUA_REGISTRYINDEX);
    if (!lua_islightuserdata(L, -1)) {
        lua_pop(L, 1);
        return nullptr;
    }
    void *userdata = lua_tolightuserdata(L, -1);
    lua_pop(L, 1);
    return reinterpret_cast<godot::LuauVM*>(userdata);
}

using namespace godot;


// Source: https://github.com/Fumohouse/godot-luau-script
// Based on the default implementation seen in the Lua 5.1 reference
static void *lua_alloc(void *, void *ptr, size_t, size_t nsize) {
    if (nsize == 0) {
        // Lua assumes free(NULL) is ok. For Godot it is not.
        if (ptr)
            memfree(ptr);

        return nullptr;
    }

    return memrealloc(ptr, nsize);
}

static lua_CompileOptions luau_vm_compile_options = {
    // int optimizationLevel;
    1,

    // int debugLevel;
    1,

    // int coverageLevel;
    0,

    // const char* vectorLib;
    nullptr,

    // const char* vectorCtor;
    "vector",

    // const char** mutableGlobals;
};

LuauVM::LuauVM() {
    L = lua_newstate(lua_alloc, nullptr);
    lua_setnode(L, this);
    create_metatables();
}

LuauVM::~LuauVM() {
    lua_close(L);
}

void LuauVM::_bind_methods() {
    ClassDB::bind_method(D_METHOD("load_string", "code", "chunkname"), &LuauVM::load_string, DEFVAL("loadstring"));
    ClassDB::bind_method(D_METHOD("do_string", "code", "chunkname"), &LuauVM::do_string, DEFVAL("dostring"));

    ClassDB::bind_method(D_METHOD("open_libraries", "libraries"), &LuauVM::open_libraries);
    ClassDB::bind_method(D_METHOD("open_all_libraries"), &LuauVM::open_all_libraries);

    _bind_passthrough_methods();

    ADD_SIGNAL(MethodInfo("stdout", PropertyInfo(Variant::STRING, "message")));
}

int metatable_object__eq(lua_State *L) {
    Variant var1 = lua_toobject(L, 1);
    Variant var2 = lua_toobject(L, 2);
    if (var1.get_type() != Variant::Type::OBJECT && var2.get_type() != Variant::Type::OBJECT) {
        ::lua_pushboolean(L, ::lua_equal(L, 1, 2));
        return 1;
    }
    godot::Object *object1 = var1.operator godot::Object *();
    godot::Object *object2 = var2.operator godot::Object *();
    ::lua_pushboolean(L, object1->get_instance_id() == object2->get_instance_id());
    return 1;
}

int metatable_instance__index(lua_State *L) {
    
}

void LuauVM::create_metatables() {
    luaL_newmetatable("object");
    
    lua_setuserdatadtor(L, 1, object_userdata_dtor);

    ::lua_pushstring(L, "object");
    ::lua_rawsetfield(L, -2, "__type");

    ::lua_pushcfunction(L, metatable_object__eq, NULL);
    ::lua_rawsetfield(L, -2, "__eq");


    luaL_newmetatable("Instance");

    ::lua_pushstring(L, "Instance");
    ::lua_rawsetfield(L, -2, "__type");
    
    ::lua_pushcfunction(L,metatable_instance__index, NULL);
    ::lua_rawsetfield(L, -2, "__index");
    (lua_pop)(2);
}



static int godot_print(lua_State* L) {
    LuauVM *node = lua_getnode(L);
    int nargs = node->lua_gettop();

    String s = String();
    for (int i = 1; i <= nargs; i++) {
        String ss;
        if (node->lua_isnumber(i) || node->lua_isstring(i))
            ss = (node->lua_tostring)(i);
        else {
            (node->lua_getglobal)("tostring"); // Push global "tostring"
            node->lua_pushvalue(i); // Push argument

            int err = node->lua_pcall(1, 1, 0); // Call tostring

            if (err != LUA_OK) {
                lua_error(L);
                return 0;
            }

            ss = (node->lua_tostring)(-1);
            (node->lua_pop)(1); // Pop tostring'ed argument
        }

        s += ss;
        if (i < nargs) s += '\t';
    }

    node->emit_signal("stdout", s);
    return 0;
}


int lua_loadstring(lua_State* L)
{
    size_t l = 0;
    const char* s = luaL_checklstring(L, 1, &l);
    const char* chunkname = luaL_optstring(L, 2, s);

    lua_setsafeenv(L, LUA_ENVIRONINDEX, false);

    size_t bytecode_size = 0;
    char *bytecode = luau_compile(s, l, &luau_vm_compile_options, &bytecode_size);
    if (luau_load(L, chunkname, bytecode, bytecode_size, 0) == 0)
        return 1;

    lua_pushnil(L);
    lua_insert(L, -2); // put before error message
    return 2;          // return nil plus error message
}


int luaopen_base_luau(lua_State *L) {
    int nret = luaopen_base(L);
    ::lua_pushcfunction(L, godot_print, "print");
    ::lua_rawsetfield(L, LUA_GLOBALSINDEX, "print");
    ::lua_pushcfunction(L, lua_loadstring, "loadstring");
    ::lua_rawsetfield(L, LUA_GLOBALSINDEX, "loadstring");
    return nret;
}


static const luaL_Reg lualibs[] = {
    {"", luaopen_base_luau},
    {LUA_COLIBNAME, luaopen_coroutine},
    {LUA_TABLIBNAME, luaopen_table},
    {LUA_OSLIBNAME, luaopen_os},
    {LUA_STRLIBNAME, luaopen_string},
    {LUA_MATHLIBNAME, luaopen_math},
    {LUA_VECLIBNAME, luaopen_vector},
    {LUA_DBLIBNAME, luaopen_debug},
    {LUA_UTF8LIBNAME, luaopen_utf8},
    {LUA_BITLIBNAME, luaopen_bit32},
    {NULL, NULL},
};


void LuauVM::open_libraries(const Array &libraries) {
    const luaL_Reg* lib = lualibs;
    for (; lib->func; lib++)
    {
        if (!libraries.has(lib->name))
            continue;
        lua_pushcfunction(L, lib->func, NULL);
        lua_pushstring(lib->name);
        lua_call(1, 0);
    }
}

void LuauVM::open_all_libraries() {
    const luaL_Reg* lib = lualibs;
    for (; lib->func; lib++)
    {
        lua_pushcfunction(L, lib->func, NULL);
        lua_pushstring(lib->name);
        lua_call(1, 0);
    }
    lua_pushvalue(L, LUA_REGISTRYINDEX); // debug.getregistry().OS_CLOCK = _G.os_clock
    lua_pushglobaltable(L);
    lua_getfield(L, -1, "os");
    lua_remove(-2);
    lua_getfield(L, -1, "clock");
    lua_remove(-2);
    lua_setfield(L, -2, "OS_CLOCK");
    lua_pop(L, 1);
}


int64_t LuauVM::get_memory_usage_bytes() {
    return lua_gc(LUA_GCCOUNTB, 0) + 1024 * lua_gc(LUA_GCCOUNT, 0);;
}


int LuauVM::load_string(const String &code, const String &chunkname) {
    auto utf8 = code.ascii();
    auto source = utf8.get_data();
    size_t bytecode_size = 0;
    char *bytecode = luau_compile(source, strlen(source), &luau_vm_compile_options, &bytecode_size);
    if (bytecode == nullptr)
        return -1;
    
    int status = luau_load(L, chunkname.ascii().get_data(), bytecode, bytecode_size, 0);
    std::free(bytecode);
    return status;
}

int LuauVM::do_string(const String &code, const String &chunkname) {
    int status = load_string(code, chunkname);
    if (status != LUA_OK)
        return status;
    status = lua_pcall(0, LUA_MULTRET, 0);
    return status;
}

// task scheduler
// NOTE: task_synchronize and task_desynchronize do not work yet.
// TODO: add em

// task_defer_spawn, task_defer_delay, task_defer_resume

/* TASK SCHEDULER LOOP:
 GODOT call_deferred:
  - process inputs
  - do rendering: can be forced with RenderingServer.force_draw()
  - replication poll
  - run task_throttled_spawn,task_throttled_delay,task_throttled_wait
*/


int LuauVM::task_create(lua_State *L) {
    int nargs = ::lua_gettop(L);
    lua_State *thr;
    int nres,status;
    if (nargs == 0) {
        ::lua_pushstring(L, "task.spawn expected 1 or more arguments, got 0");
        return ::lua_error(L);
    }
    nargs--;
    bool isnew = false;
    if (::lua_isfunction(L, 1)) {
        thr = ::lua_newthread(L);
        ::lua_insert(L, 1);
        ::lua_pushvalue(L, 2); // move function from stack 2 to -1
        ::lua_remove(L, 2); 
        ::lua_xmove(L, thr, nargs+1); // all args + function
        isnew = true;
    } else if (::lua_isthread(L, 1)) {
        thr = ::lua_tothread(L, 1); // no function specified
    } else {
        ::lua_pushfstring(L, "functionOrThread expected function|thread, got %s", ::lua_typename(L,::lua_type(L,1)));
        return ::lua_error(L);
    }
    // all args start at 2
    if (isnew) {
        status = ::lua_resume(thr, L, nargs, &nres);
    } else if (::lua_costatus(L, thr) == LUA_COSUS) {
        status = ::lua_resume(thr, L, nargs, &nres);
    } else {
        ::lua_pushstring(L, "cannot resume thread");
        return ::lua_error(L);
    }
    if (status != LUA_YIELD and status != LUA_BREAK and status != LUA_OK) handle_error(L, thr);
    ::lua_pop(L, nres);
    return 1; // the thread
}
int LuauVM::task_defer(lua_State *L) {
    int nargs = ::lua_gettop(L);
    bool isnew = false;
    lua_State* thr;
    int status;
    if (nargs == 0) {
        ::lua_pushstring(L, "task.defer expected 1 or more arguments, got 0");
        return ::lua_error(L);
    }
    if (::lua_isfunction(L, 1)) {
        thr = ::lua_newthread(L);
        ::lua_pushvalue(L, 1);
        ::lua_remove(L, 1);
        ::lua_xmove(L, thr, 1);
        ::lua_insert(L, 1); // thread is now arg 1
        isnew = true;
    } else if (::lua_isthread(L, 1)) {
        thr = ::lua_tothread(L, 1);
        if (::lua_costatus(L, thr) != LUA_COSUS) {
            ::lua_pushstring(L, "cannot resume thread");
            return ::lua_error(L);
        }
    } else {
        ::lua_pushfstring(L, "functionOrThread expected function|thread, got %s", ::lua_typename(L, ::lua_type(L, 1)));
        return ::lua_error(L);
    }
    // append to registry.task_defer_spawn the arguments
    ::lua_pushvalue(L, LUA_REGISTRYINDEX);
    ::lua_getfield(L, -1, "task_defer_spawn");
    ::lua_objlen(L, -1);
    ::lua_remove(L, -3); // pop LUA_REGISTRYINDEX
    ::lua_newtable(L);
    for (int i = 1; i < nargs+1; i++) {
        ::lua_pushinteger(L, i);
        ::lua_pushvalue(L, i);
        ::lua_settable(L, -3);
    }
    ::lua_settable(L, -3);
    ::lua_pop(L, 1); // pop task_defer_spawn
    ::lua_pop(L, nargs-1); // pop all arguments except the thread
    return 1; // return thread
}
int LuauVM::task_delay(lua_State *L) {
    int nargs = ::lua_gettop(L);
    lua_State *thr;
    if (nargs == 0) {
        ::lua_pushstring(L, "functionOrThread expected function|thread, got nil.");
        return ::lua_error(L);
    } else if (nargs == 1) {
        ::lua_pushstring(L, "delay expected number, got nil.");
        return ::lua_error(L);
    }
    if (::lua_isfunction(L, 1)) {
        thr = ::lua_newthread(L);
        ::lua_pushvalue(L, 1); // copy function from index 1 to top of stack
        ::lua_xmove(L, thr, 1); // move function to thr
        ::lua_remove(L, 1); // remove function
        ::lua_insert(L, 1); // move thread to 1
    } else if (::lua_isthread(L, 1)) {
        if (::lua_costatus(L, thr) == LUA_COSUS) {
            ::lua_pushstring(L, "cannot resume thread");
            return ::lua_error(L);
        }
    } else {
        ::lua_pushfstring(L, "functionOrThread expected function|thread, got %s", ::lua_typename(L,::lua_type(L,1)));
        return ::lua_error(L);
    }
    if (!::lua_isnumber(L, 2)) {
        ::lua_pushfstring(L, "delay expected number, got %s", ::lua_typename(L,::lua_type(L,2)));
        return ::lua_error(L);
    }
    // thr + duration + start args current stack
    ::lua_pushvalue(L, LUA_REGISTRYINDEX); 
    ::lua_getfield(L, -1, "task_defer_delay");
    ::lua_objlen(L, -1);
    ::lua_remove(L, -3); // pop LUA_REGISTRYINDEX
    ::lua_newtable(L); // table, key, value pair generated
    ::lua_pushvalue(L, LUA_REGISTRYINDEX);
    ::lua_pushinteger(L, 1);
    ::lua_getfield(L, -2, "task_serialized");
    ::lua_settable(L, -4); // add to arguments table task_serialized state, removes key value pair
    ::lua_pop(L, 1);  // pop LUA_REGISTRYINDEX
    ::lua_pushinteger(L, 3); // write when_it_started as 2nd argument
    ::lua_pushvalue(L, LUA_REGISTRYINDEX);
    ::lua_getfield(L, -1, "OS_CLOCK");
    ::lua_remove(L, -2); // remove registry index
    ::lua_call(L, 0, 1); // replace os.clock() with the current time
    ::lua_settable(L, -3); 
    ::lua_pushinteger(L, 2); // write delay as 3rd argument
    ::lua_pushvalue(L, 2);
    ::lua_settable(L, -3);
    ::lua_pushinteger(L, 4); // write thread as 4th argument
    ::lua_pushvalue(L, 1);
    ::lua_settable(L, -3);
    for (int i = 3; i < nargs+1; i++) { // write rest of args
        ::lua_pushinteger(L, i+2);
        ::lua_pushvalue(L, i);
        ::lua_settable(L, -3);
    }
    ::lua_settable(L, -3);// push to task_defer delay
    ::lua_pop(L, nargs); // pop all but the thread
    return 1; // return thread
}
int LuauVM::task_desynchronize(lua_State *L) {
    int nargs = ::lua_gettop(L);
    if (nargs > 0) {
        ::lua_pushfstring(L, "expected 0 arguments, got %d", nargs);
        return ::lua_error(L);
    }
    return 0;
}
int LuauVM::task_synchronize(lua_State *L) {
    int nargs = ::lua_gettop(L);
    if (nargs > 0) {
        ::lua_pushfstring(L, "expected 0 arguments, got %d", nargs);
        return ::lua_error(L);
    }
    return 0;
}
int LuauVM::task_wait(lua_State* L) {
    int nargs = ::lua_gettop(L);
    lua_Number wait_time;
    if (nargs > 1) {
        ::lua_pushfstring(L, "expected 1 argument, got %d", nargs);
        return ::lua_error(L);
    } else if (nargs == 1) {
        if (::lua_isnil(L, 1)) {
            wait_time = 0;
        } else if (!::lua_isnumber(L, 1)) {
            ::lua_pushfstring(L, "duration expected number, got %s", ::lua_typename(L,::lua_type(L,1)));
            return ::lua_error(L);
        } else wait_time = ::lua_tonumber(L, 1);
        ::lua_pop(L, 1);
    } else wait_time = 0;
    /*
        function task.wait(duration)
            local task_defer_resume = debug.getregistry().task_defer_resume
            task_defer_resume[#task_defer_resume+1]={debug.getregistry().task_is_in_sync,os.clock(),duration,coroutine.running()}
            coroutine.yield()
        end
    */
    ::lua_pushvalue(L, LUA_REGISTRYINDEX); 
    ::lua_getfield(L, -1, "task_defer_resume");
    ::lua_objlen(L, -1);
    ::lua_remove(L, -3);
    ::lua_newtable(L);
    ::lua_pushinteger(L, 1);
    ::lua_pushvalue(L, LUA_REGISTRYINDEX);
    ::lua_getfield(L, -1, "OS_CLOCK");
    ::lua_remove(L,-2);
    ::lua_call(L, 0, 1);
    ::lua_settable(L, -3);
    ::lua_pushinteger(L, 2);
    ::lua_pushnumber(L, wait_time);
    ::lua_settable(L,-3);
    ::lua_pushinteger(L, 3);
    ::lua_pushthread(L);
    ::lua_settable(L,-3);
    ::lua_settable(L,-3);
    ::lua_pop(L, 1);
    return ::lua_yield(L, 0);
}
int LuauVM::task_cancel(lua_State* L) {
    int nargs = ::lua_gettop(L);
    if (nargs != 1) {
        ::lua_pushfstring(L, "expected 1 argument, got %d", nargs);
        return ::lua_error(L);
    } else if (!::lua_isthread(L, 1)) {
        ::lua_pushfstring(L, "thread expected thread, got %s", ::lua_typename(L,::lua_type(L,1)));
        return ::lua_error(L);
    }
    ::lua_State* thread = ::lua_tothread(L, -1);
    int status = ::lua_costatus(L, thr);
    if (status == LUA_CORUN or status == LUA_CONOR) { // running/normal cant be cancelled
        ::lua_pushstring(L, "cannot cancel task");
        return ::lua_error(L);
    } else {
        ::lua_closethread(thread, L);
    }
    ::lua_pop(L,1);
    return 0;
}
void LuauVM::handle_error(lua_State* thr) {
    //TODO: Print error
    ::lua_closethread(thr, L);
}
void LuauVM::terminate_error(lua_State* thr) {
    //TODO: Print error
    ::lua_closethread(thr, L);
}
bool LuauVM::task_resumption_cycle(bool terminate = false) {
    int k = 1;
    lua_Number curr_os_clock;
    bool is_serialized_res_cycle;
    ::lua_pushvalue(L, LUA_REGISTRYINDEX);
    ::lua_getfield(L, -1, "OS_CLOCK");
    ::lua_call(L, 0, 1);
    curr_os_clock = ::lua_tonumber(L, -1);
    ::lua_pop(L, 1);
    ::lua_getfield(L, -1, "task_serialized");
    is_serialized_res_cycle = ::lua_toboolean(L, -1);
    ::lua_pop(L, 1);
    ::lua_getfield(L, -1, "task_defer_spawn");
    ::lua_newtable(L); 
    ::lua_setfield(L, -3, "task_defer_spawn"); // clear table with empty clone
    // current stack status: registry, task_defer_spawn_old_tbl
    while (true) {
        ::lua_geti(L, -1, k++); // get the table or nil
        if (::lua_isnil(L, -1)) break;
        ::lua_geti(L, -1, 1);
        lua_State* thr = ::lua_tothread(L, -1);
        if (::lua_costatus(L, thr) != LUA_COSUS) {
            ::lua_pop(L, 2); // thread and the table containing the thread
        } else {
            // load all values
            int tk,status,nargs,nres = 0;
            tk = 2;
            while (true) {
                ::lua_geti(L, -2-nargs, tk++);
                if (::lua_isnil(L, -1)) break;
                nargs++;
            }
            status = ::lua_resume(thr, L, nargs, &nres);
            if (status == LUA_ERRERR or status == LUA_ERRMEM or status == LUA_ERRRUN or status == LUA_ERRSYNTAX) handle_error(thr);
            else ::lua_pop(L, nres);
            ::lua_pop(L, 2); // pop table and thread
        }
    }
    ::lua_pop(L ,2); // pop the table and the key
    ::lua_getfield(L, -1, "task_wait_delay");
    ::lua_newtable(L); 
    ::lua_setfield(L, -3, "task_wait_delay"); // clear table with empty clone
    // current stack status: task_wait_delay_old_tbl
    while (true) {
        ::lua_geti(L, -1, k++); // get the table or nil
        if (::lua_isnil(L, -1)) break;
        ::lua_geti(L, -1, 1);
        bool is_serialized = ::lua_toboolean(L, -1);
        if (is_serialized != is_serialized_res_cycle) {
            ::lua_pop(L, 2);
            continue;
        } else ::lua_pop(L, 1);
        lua_Number duration, when_it_started;
        ::lua_geti(L, -1, 2);
        ::lua_geti(L, -2, 3);
        duration = ::lua_tonumber(L, -1);
        when_it_started = ::lua_tonumber(L, -2);
        ::lua_pop(L, 2);
        ::lua_geti(L, -1, 4);
        lua_State* thr = ::lua_tothread(L, -1);
        if (curr_os_clock<duration+when_it_started) {
            ::lua_pop(L, 2); // the thread and the table
        } else if (::lua_costatus(L, thr) != LUA_COSUS) {
            ::lua_pop(L, 2); // thread and the table containing the thread
        } else {
            // load all values
            int tk,status,nargs,nres = 0;
            tk = 5;
            while (true) {
                ::lua_geti(L, -2-nargs, tk++);
                if (::lua_isnil(L, -1)) break;
                nargs++;
            }
            if (terminate) {
                terminate_error(thr);
                ::lua_pop(L, nargs);
            } else {
                status = ::lua_resume(thr, L, nargs, &nres);
            }
            if (status == LUA_ERRERR or status == LUA_ERRMEM or status == LUA_ERRRUN or status == LUA_ERRSYNTAX) handle_error(thr);
            else ::lua_pop(L, nres);
            ::lua_pop(L, 2); // pop table and thread
        }
    }
    ::lua_pop(L ,2); // pop the table and the key
    ::lua_getfield(L, -1, "task_wait_resume");
    ::lua_newtable(L); 
    ::lua_setfield(L, -3, "task_wait_resume"); // clear table with empty clone
    // current stack status: task_wait_resume_old_tbl
    while (true) {
        ::lua_geti(L, -1, k++); // get the table or nil
        if (::lua_isnil(L, -1)) break;
        ::lua_geti(L, -1, 1);
        bool is_serialized = ::lua_toboolean(L, -1);
        if (is_serialized != is_serialized_res_cycle) {
            ::lua_pop(L, 2);
            continue;
        } else ::lua_pop(L, 1);
        lua_Number duration, when_it_started;
        ::lua_geti(L, -1, 2);
        ::lua_geti(L, -2, 3);
        duration = ::lua_tonumber(L, -1);
        when_it_started = ::lua_tonumber(L, -2);
        ::lua_pop(L, 2);
        ::lua_geti(L, -1, 4);
        lua_State* thr = ::lua_tothread(L, -1);
        if (curr_os_clock<duration+when_it_started) {
            ::lua_pop(L, 2); // the thread and the table
        } else if (::lua_costatus(L, thr) != LUA_COSUS) {
            ::lua_pop(L, 2); // thread and the table containing the thread
        } else {
            // load all values
            int tk,status,nres = 0;
            if (terminate) {
                terminate_error(thr);
                ::lua_pop(L, nargs);
            } else {
                ::lua_pushnumber(L, curr_os_clock-when_it_started);
                status = ::lua_resume(thr, L, 1, &nres);
            }
            if (status == LUA_ERRERR or status == LUA_ERRMEM or status == LUA_ERRRUN or status == LUA_ERRSYNTAX) handle_error(thr);
            else ::lua_pop(L, nres);
            ::lua_pop(L, 2); // pop table and thread
        }
    }
}