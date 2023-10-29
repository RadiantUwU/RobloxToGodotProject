#include "rblx_events.hpp"
#include "rblx_main.hpp"
#include <lua.h>

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
    long long hash = fn.as_pointer_hash(-1); // get closure hash
    fn.push_ref(signal->ref); // push table
    fn.push_value(-2); // push the closure
    fn.rawset(-2, hash);
    fn.pop_stack(2); // pop table and closure
    RBXScriptConnection* connection = fn.new_userdata<RBXScriptConnection>(fn.UD_TRBXSCRIPTCONNECTION);
    connection->signal = signal;
    connection->ref = hash;
    return 1; // returns the connection
}
int RBXScriptSignal::lua_once(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_usertype_argument(1,"self", fn.UD_TRBXSCRIPTSIGNAL);
    fn.assert_type_argument(2,"callback",LUA_TFUNCTION);
    RBXScriptSignal* signal = fn.as_userdata<RBXScriptSignal>(1);
    fn.push_object((long long)0);
    fn.push_object((long long)signal->ref);
    fn.push_cclosure(RBXScriptSignal::lua_onfire_autodisconnect, 3); // grabs the callback
    long long hash = fn.as_pointer_hash(-1); // get closure hash
    fn.push_object(hash);
    fn.setupvalue(-2, 2);
    fn.push_ref(signal->ref); // push table
    fn.push_value(-2); // push the closure
    fn.rawset(-2, hash);
    fn.pop_stack(2); // pop table and closure
    RBXScriptConnection* connection = fn.new_userdata<RBXScriptConnection>(fn.UD_TRBXSCRIPTCONNECTION);
    connection->signal = signal;
    connection->ref = hash;
    return 1; // returns the connection
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
    long long hash = fn.as_pointer_hash(-1); // get closure hash
    fn.push_ref(signal->ref); // push table
    fn.push_value(-2); // push the closure
    fn.rawset(-2, hash);
    fn.pop_stack(2); // pop table and closure
    fn.yield(0);
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
        for (int i = 0; i < args; i++) 
            iter.push_value(2+i);
        iter.call(args,0);
    }
    return 0;
}
int RBXScriptSignal::lua_onfire(lua_State *L) {
    luau_function_context fn = L;
    int args = fn.get_stack_size();
    delete fn.getinfo("f",1);
    fn.getupvalue(-1, 1);
    fn.remove_stack(-2);
    fn.insert_into(1);
    fn.new_thread(args);
    fn.resume(0, 0, 0);
    return 0;
}
int RBXScriptSignal::lua_onfire_autodisconnect(lua_State *L) {
    luau_function_context fn = L;
    int args = fn.get_stack_size();
    delete fn.getinfo("f",1);
    fn.getupvalue(-1, 2);
    long long hash = (long long)fn.to_object();
    fn.getupvalue(-1, 3);
    long long ref = (long long)fn.to_object();
    fn.getupvalue(-1, 1);
    fn.remove_stack(-2);
    fn.insert_into(1);
    fn.new_thread(args);
    fn.push_ref(ref);
    fn.push_object();
    fn.set(-1,hash);
    fn.pop_stack(1);
    fn.resume(0, 0, 0);
    return 0;
}
int RBXScriptSignal::lua_resume_onfire(lua_State *L) {
    luau_function_context fn = L;
    int args = fn.get_stack_size();
    delete fn.getinfo("f",1);
    fn.getupvalue(-1, 1);
    fn.remove_stack(-2);
    fn.insert_into(1);
    fn.resume(args,0,0);
    return 0;
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
    return 1;
}
int RBXScriptConnection::lua_Disconnect(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_usertype_argument(1, "self", fn.UD_TRBXSCRIPTCONNECTION);
    RBXScriptConnection* conn = fn.as_userdata<RBXScriptConnection>(-1);
    conn->Disconnect();
    return 0;
}

};