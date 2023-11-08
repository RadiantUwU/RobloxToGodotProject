#include "rblx_basic_types.hpp"
#include "rblx_script.hpp"
#include "rblx_main.hpp"
#include "rblx_instance.hpp"
#include "rblx_debug.hpp"

namespace godot {

bool LuaSourceContainer::has_property(const LuaString& s, bool recurse = true) const {
    if (s == "RuntimeSource") {
        return true;
    } else 
        return (recurse) ? Instance::has_property(s, recurse) : false;
    
}
int LuaSourceContainer::lua_get(lua_State *L) {
    luau_function_context fn = L;
    const LuaString s = (LuaString)fn.as_object(2);
    if (s == "RuntimeSource") {
        if (VM->context <= RBLX_VMRunContext::RUNCTXT_PLUGIN) {
            fn.push_object(RuntimeSource);
            return 1;
        }
        fn.error("Expected permission level >=PLUGIN for protected property RuntimeSource");
    } else 
        return Instance::lua_get(L);
}
int LuaSourceContainer::lua_set(lua_State *L) {
    luau_function_context fn = L;
    const LuaString s = (LuaString)fn.as_object(2);
    if (s == "RuntimeSource") {
        if (VM->context <= RBLX_VMRunContext::RUNCTXT_PLUGIN) {
            fn.assert_type_argument(3, "value", LUA_TSTRING);
            const LuaString v = (LuaString)fn.as_object(3);
            bool should_fire = RuntimeString != v;
            RuntimeString = v;
            if (should_fire) {
                this->Changed->Fire("RuntimeString");
                if (this->property_signals.has("RuntimeString")) {
                    this->property_signals.get("RuntimeString")->Fire(v);
                }
            }
            return 0;
        }
        fn.error("Expected permission level >=PLUGIN for protected property RuntimeSource");
    } else 
        return Instance::lua_get(L);
    
}

bool BaseScript::has_property(const LuaString& s, bool recurse = true) const {
    if (s == "Enabled") {
        return true;
    } else if (s == "Disabled") {
        return true
    } else if (s == "LinkedSource") {
        return true
    } else if (s == "RunContext") {
        return true
    } else 
        return (recurse) ? Instance::has_property(s, recurse) : false;
}
int BaseScript::lua_get(const LuaString &s) {
    luau_function_context fn = L;
    const LuaString s = (LuaString)fn.as_object(2);
    if (s == "Disabled") {
        fn.push_object(!isEnabled());
        return 1;
    } else if (s == "Enabled") {
        fn.push_object(isEnabled());
        return 1;
    } else if (s == "LinkedSource") {
        fn.push_object(LinkedSource);
        return 1;
    } else if (s == "RunContext") {
        fn.push_object(RunContext); // enum conversion not added yet
        return 1;
    } else 
        return LuaSourceContainer::lua_get(L);
}
int BaseScript::lua_set(const LuaString &s) {
    luau_function_context fn = L;
    const LuaString s = (LuaString)fn.as_object(2);
    if (s == "Disabled") {
        fn.assert_type_argument(3,"value",LUA_TBOOLEAN);
        const bool b = (bool)fn.as_object(3);
        setEnable(!b);
        return 0;
    } else if (s == "Enabled") {
        fn.assert_type_argument(3,"value",LUA_TBOOLEAN);
        const bool b = (bool)fn.as_object(3);
        setEnable(b);
        return 0;
    } else if (s == "LinkedSource") {
        fn.assert_type_argument(3,"value",LUA_TSTRING);
        const LuaString v = (LuaString)fn.as_object(3);
        
        return 0;
    } else if (s == "RunContext") {
        fn.assert_usertype_argument(3,"value",fn.UD_TENUMITEM);
        //const int v = fn.as_userdata(3);
        //fn.push_object(RunContext); // enum conversion not added yet
        return 0;
    } else 
        return LuaSourceContainer::lua_set(L);
}

};