#ifndef RBLX_EVENTS
#define RBLX_EVENTS

#include <godot_cpp/variant/variant.hpp>
#include <lua.h>
#include <lualib.h>
#include <godot_cpp/templates/vector.hpp>

namespace godot {

class RBXScriptConnection final {
    lua_State *L;
    RBXScriptSignal* signal;
    RBXScriptConnection();
public:
    ~RBXScriptConnection();
    bool isConnected();
    void Disconnect();
};

class RBXScriptSignal final {
    lua_State *L;
    int ref;
    friend class RBXScriptConnection;
    void Disconnect()
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

}

#endif