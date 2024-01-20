#include "rblx_events.hpp"
#include "rblx_debug.hpp"
#include "rblx_main.hpp"
#include "rblx_script.hpp"
#include <lua.h>
#include <cstddef>
#include <cstring>

namespace godot {

RBXScriptSignal::RBXScriptSignal(lua_State *L) {
    luau_context ls = luau_context(L);
    ls.new_table();
    ref = ls.new_ref(-1);
    this->L = L;
}
RBXScriptSignal::RBXScriptSignal(luau_State *L) {
    luau_context lc = luau_context(L);
    lc.new_table();
    ref = lc.new_ref(-1);
    this->L = L->get_state();
}
RBXScriptSignal::~RBXScriptSignal() {
    luau_context lc = luau_context(L);
    lc.delete_ref(ref);
}
int RBXScriptSignal::lua_connect(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_usertype_argument(1,"self", fn.UD_TRBXSCRIPTSIGNAL);
    fn.assert_type_argument(2,"callback",LUA_TFUNCTION);
    RBXScriptSignal* signal = fn.as_userdata<RBXScriptSignal>(1);
    fn.push_cclosure(RBXScriptSignal::lua_onfire, 1); // grabs the callback
    int64_t hash = fn.as_pointer_hash(-1); // get closure hash
    fn.push_ref(signal->ref); // push table
    fn.push_value(-2); // push the closure
    fn.rawset(-2, hash);
    fn.pop_stack(2); // pop table and closure
    RBXScriptConnection* connection = fn.new_userdata<RBXScriptConnection>(fn.UD_TRBXSCRIPTCONNECTION);
    connection->signal = signal;
    connection->ref = hash;
    return fn.lua_return(1); // returns the connection
}
int RBXScriptSignal::lua_once(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_usertype_argument(1,"self", fn.UD_TRBXSCRIPTSIGNAL);
    fn.assert_type_argument(2,"callback",LUA_TFUNCTION);
    RBXScriptSignal* signal = fn.as_userdata<RBXScriptSignal>(1);
    fn.push_object((int64_t)0);
    fn.push_object((int64_t)signal->ref);
    fn.push_cclosure(RBXScriptSignal::lua_onfire_autodisconnect, 3); // grabs the callback
    int64_t hash = fn.as_pointer_hash(-1); // get closure hash
    fn.push_object(hash);
    fn.setupvalue(-2, 2);
    fn.push_ref(signal->ref); // push table
    fn.push_value(-2); // push the closure
    fn.rawset(-2, hash);
    fn.pop_stack(2); // pop table and closure
    RBXScriptConnection* connection = fn.new_userdata<RBXScriptConnection>(fn.UD_TRBXSCRIPTCONNECTION);
    connection->signal = signal;
    connection->ref = hash;
    return fn.lua_return(1); // returns the connection
}
int RBXScriptSignal::lua_connectparallel(lua_State *L) {
    return RBXScriptSignal::lua_connect(L); // TODO: implement parallel stuff
}
int RBXScriptSignal::lua_wait(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_usertype_argument(1,"self", fn.UD_TRBXSCRIPTSIGNAL);
    RBXScriptSignal* signal = fn.as_userdata<RBXScriptSignal>(1);
    fn.push_current_thread();
    fn.push_cclosure(RBXScriptSignal::lua_resume_onfire, 1); // grabs the callback
    int64_t hash = fn.as_pointer_hash(-1); // get closure hash
    fn.push_ref(signal->ref); // push table
    fn.push_value(-2); // push the closure
    fn.rawset(-2, hash);
    fn.pop_stack(2); // pop table and closure
    return fn.yield(0);
}
int RBXScriptSignal::lua_fire(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_usertype_argument(1,"self", fn.UD_TRBXSCRIPTSIGNAL);
    int args = fn.get_stack_size()-1;
    RBXScriptSignal* signal = fn.as_userdata<RBXScriptSignal>(1);
    fn.push_ref(signal->ref); // push table
    fn.clone_table();
    int k = 0;
    while (true) {
        luau_context iter = L;
        k = iter.rawiter(-1, k);
        if (k == -1) break;
        for (int i = 0; i < args; i++) 
            iter.push_value(2+i);
        iter.call(args,0);
    }
    RBLX_PRINT_VERBOSE("No error in lua_fire!");
    return fn.lua_return(0);
}
int RBXScriptSignal::lua_onfire(lua_State *L) {
    luau_function_context fn = L;
    int args = fn.get_stack_size();
    delete fn.getinfo("f",1);
    fn.getupvalue(-1, 1);
    fn.remove_stack(-2);
    fn.insert_into(1);
    fn.new_thread(args);
    fn.resume(0, 0);
    RBLX_PRINT_VERBOSE("No error in lua_onfire!");
    return fn.lua_return(0);
}
int RBXScriptSignal::lua_onfire_autodisconnect(lua_State *L) {
    luau_function_context fn = L;
    int args = fn.get_stack_size();
    delete fn.getinfo("f",1);
    fn.getupvalue(-1, 2);
    int64_t hash = (int64_t)fn.to_object();
    fn.getupvalue(-1, 3);
    int64_t ref = (int64_t)fn.to_object();
    fn.getupvalue(-1, 1);
    fn.remove_stack(-2);
    fn.insert_into(1);
    fn.new_thread(args);
    fn.push_ref(ref);
    fn.push_object();
    fn.set(-1,hash);
    fn.pop_stack(1);
    fn.resume(0, 0);
    RBLX_PRINT_VERBOSE("No error in lua_onfire_autodisconnect!");
    return fn.lua_return(0);
}
int RBXScriptSignal::lua_resume_onfire(lua_State *L) {
    luau_function_context fn = L;
    int args = fn.get_stack_size();
    delete fn.getinfo("f",1);
    fn.getupvalue(-1, 1);
    fn.remove_stack(-2);
    fn.insert_into(1);
    fn.resume(args,0);
    RBLX_PRINT_VERBOSE("No error in lua_resume_onfire!");
    return fn.lua_return(0);
}
int RBXScriptSignal::lua_get(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_usertype_argument(1, "self", fn.UD_TRBXSCRIPTSIGNAL);
    fn.assert_type_argument(2, "key", LUA_TSTRING);
    RBXVariant v = fn.to_object();
    const char *s = v.get_str();
    if (strcmp(s,"Connect") == 0) {
        fn.push_object(&RBXScriptSignal::lua_connect,"RBXScriptSignal::Connect");
        return fn.lua_return(1);
    } else if (strcmp(s, "Once") == 0) {
        fn.push_object(&RBXScriptSignal::lua_once,"RBXScriptSignal::Once");
        return fn.lua_return(1);
    } else if (strcmp(s, "ConnectParallel") == 0) {
        fn.push_object(&RBXScriptSignal::lua_connectparallel,"RBXScriptSignal::ConnectParallel");
        return fn.lua_return(1);
    } else if (strcmp(s, "Wait") == 0) {
        fn.push_object(&RBXScriptSignal::lua_wait,"RBXScriptSignal::Wait");
        return fn.lua_return(1);
    }
    fn.error("invalid key provided to RBXScriptSignal.__index()");
}
void RBXScriptSignal::lua_destroy(lua_State *L, void *ud) {
    RBXScriptSignal* signal = (RBXScriptSignal*)ud;
    signal->~RBXScriptSignal();
}

bool RBXScriptConnection::isConnected() {
    luau_context ls = signal->L;
    ls.push_ref(signal->ref);
    return ls.rawget(-1,ref) != LUA_TNIL;
}
void RBXScriptConnection::Disconnect() {
    luau_context ls = signal->L;
    ls.push_ref(signal->ref);
    ls.push_object();
    ls.rawset(-2,ref);
}
int RBXScriptConnection::lua_isConnected(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_usertype_argument(1, "self", fn.UD_TRBXSCRIPTCONNECTION);
    RBXScriptConnection* conn = fn.as_userdata<RBXScriptConnection>(-1);
    fn.push_object(conn->isConnected());
    return fn.lua_return(1);
}
int RBXScriptConnection::lua_Disconnect(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_usertype_argument(1, "self", fn.UD_TRBXSCRIPTCONNECTION);
    RBXScriptConnection* conn = fn.as_userdata<RBXScriptConnection>(-1);
    conn->Disconnect();
    return fn.lua_return(0);
}
int RBXScriptConnection::lua_get(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_usertype_argument(1, "self", fn.UD_TRBXSCRIPTCONNECTION);
    fn.assert_type_argument(2, "key", LUA_TSTRING);
    RBXVariant v = fn.to_object();
    const char *s = v.get_str();
    RBXScriptConnection* conn = fn.as_userdata<RBXScriptConnection>(1);
    if (strcmp(s,"Connected") == 0) {
        fn.push_object(conn->isConnected());
        return fn.lua_return(1);
    } else if (strcmp(s, "Disconnect") == 0) {
        fn.push_object(&RBXScriptConnection::lua_Disconnect,"RBXScriptConnection::Disconnect");
        return fn.lua_return(1);
    }
    fn.error("invalid key provided to RBXScriptConnection.__index()");
}

};