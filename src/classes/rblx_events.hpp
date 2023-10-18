#ifndef RBLX_EVENTS
#define RBLX_EVENTS

#include <godot_cpp/variant/variant.hpp>
#include <lua.h>
#include <lualib.h>

class RBXScriptSignal final {
    lua_State *L;
    int ref;
public:
    RBXScriptSignal();
    ~RBXScriptSignal();

    static int lua_connect(lua_State* L); // signal instance, function - return RBXScriptConnection
    static int lua_once(lua_State* L); // signal instance, function - return RBXScriptConnection
    static int lua_connectparallel(lua_State* L); // signal instance, function - return RBXScriptConnection
    static int lua_wait(lua_State* L); // signal instance - return vararg
    static int lua_fire(lua_State* L); // signal instance, vararg - return no args

    void Connect(int (*lua_cfunc)(lua_State* L));
    void Wait();
};

#endif