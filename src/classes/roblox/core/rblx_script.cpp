#include "rblx_basic_types.hpp"
#include "rblx_script.hpp"
#include "rblx_main.hpp"
#include "rblx_instance.hpp"
#include "rblx_debug.hpp"

namespace godot {

bool LuaSourceContainer::has_property(const LuaString& s, bool recurse) const {
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
            bool should_fire = RuntimeSource != v;
            RuntimeSource = v;
            if (should_fire) {
                this->Changed->Fire("RuntimeSource");
                if (this->property_signals.has("RuntimeSource")) {
                    this->property_signals.get("RuntimeSource")->Fire(v);
                }
            }
            return 0;
        }
        fn.error("Expected permission level >=PLUGIN for protected property RuntimeSource");
    } else 
        return Instance::lua_set(L);
    
}

bool BaseScript::has_property(const LuaString& s, bool recurse) const {
    if (s == "Enabled") {
        return true;
    } else if (s == "Disabled") {
        return true;
    } else if (s == "LinkedSource") {
        return true;
    } else if (s == "RunContext") {
        return true;
    } else 
        return (recurse) ? LuaSourceContainer::has_property(s, recurse) : false;
}
int BaseScript::lua_get(lua_State *L) {
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
int BaseScript::lua_set(lua_State *L) {
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
        bool should_fire = LinkedSource != v;
        LinkedSource = v;
        if (should_fire) {
            this->Changed->Fire("LinkedSource");
            if (this->property_signals.has("LinkedSource")) {
                this->property_signals.get("LinkedSource")->Fire(v);
            }
        }
        return 0;
    } else if (s == "RunContext") {
        fn.assert_usertype_argument(3,"value",fn.UD_TENUMITEM);
        //const int v = fn.as_userdata(3);
        //fn.push_object(RunContext); // enum conversion not added yet
        return 0;
    } else 
        return LuaSourceContainer::lua_set(L);
}
bool BaseScript::isEnabled() const {
    return Enabled;
}
void BaseScript::setEnable(bool enable, bool now) {
    if (enable == Enabled) return;
    Enabled = enable;
    this->Changed->Fire("Enabled");
    if (this->property_signals.has("Enabled")) {
        this->property_signals.get("Enabled")->Fire(enable);
    }
    this->Changed->Fire("Disabled");
    if (this->property_signals.has("Disabled")) {
        this->property_signals.get("Disabled")->Fire(!enable);
    }
    if (enable) {
        luau_State *L;
        if (actor == nullptr) {
            L = this->VM->main_synchronized;
        }// TODO: Implement if actor exists
        luau_context ctx = L;
        ctx.new_dictionary(1);
        ctx.push_object(this);
        ctx.rawset(-2,"script");
        ctx.push_value(-1);
        env_ref = ctx.new_ref(-1);
        add_ref(env_ref);
        ctx.new_table();
        threads_ref = ctx.new_ref(-1);
        add_ref(threads_ref);
        before_start();
        ctx.compile(Name,RuntimeSource,-1);
        ctx.new_thread(0, this);
        if (now) ctx.push_object(ctx.get_task_scheduler()->lua_task_spawn,"task::spawn",-2);
        else ctx.push_object(ctx.get_task_scheduler()->lua_task_defer,"task::defer",-2);
        ctx.call(1, 0);
    } else {
        luau_State *L;
        if (actor == nullptr) {
            L = this->VM->main_synchronized;
        }// TODO: Implement if actor exists
        luau_context ctx = L;
        ctx.push_ref(env_ref);
        ctx.clear_table();
        ctx.delete_ref(env_ref);
        remove_ref(env_ref);
        ctx.push_ref(threads_ref);
        int64_t iter = 0;
        while (true) {
            luau_context for_loop_closure = L;
            iter = ctx.rawiter(-1,iter);
            if (iter == -1) return;
            ctx.push_object(ctx.get_task_scheduler()->lua_task_cancel,"task::cancel",-2);
            ctx.call(1, 0);
        }
        ctx.clear_table();
        ctx.delete_ref(threads_ref);
        remove_ref(threads_ref);
    }
}
void BaseScript::reload() {
    if (isEnabled()) {
        setEnable(false);
        setEnable(true);
    }
}
void BaseScript::before_start() {
    if (LinkedSource != "[Embedded]" && LinkedSource != "") {
        RuntimeSource = VM->open_script_asset(LinkedSource);
    }
}
void BaseScript::_clone_object(Instance* i) const {
    BaseScript *b = (BaseScript*)i;
    b->LinkedSource = LinkedSource;
    b->RuntimeSource = RuntimeSource;
    b->RunContext = RunContext;
    luau_State *L;
    if (actor == nullptr) {
        L = VM->main_synchronized;
    }
    luau_context ctx = L;
    ctx.push_objects(VM->task->lua_task_delay, b->lua_set, .15, b, "Enabled", Enabled);
    ctx.call(5, 0);
}

bool Script::has_property(const LuaString& s, bool recurse) const {
     if (s == "Source") {
        return true;
    } else 
        return (recurse) ? BaseScript::has_property(s, recurse) : false;
}
int Script::lua_get(lua_State *L) {
    luau_function_context fn = L;
    const LuaString s = (LuaString)fn.as_object(2);
    if (s == "Source") {
        if (VM->context <= RBLX_VMRunContext::RUNCTXT_PLUGIN) {
            fn.push_object(Source);
            return 1;
        }
        fn.error("Expected permission level >=PLUGIN for protected property Source");
    } else 
        return BaseScript::lua_get(L);
}
int Script::lua_set(lua_State *L) {
    luau_function_context fn = L;
    const LuaString s = (LuaString)fn.as_object(2);
    if (s == "Source") {
        if (VM->context <= RBLX_VMRunContext::RUNCTXT_PLUGIN) {
            fn.assert_type_argument(3, "value", LUA_TSTRING);
            const LuaString v = (LuaString)fn.as_object(3);
            bool should_fire = Source != v;
            Source = v;
            if (should_fire) {
                this->Changed->Fire("Source");
                if (this->property_signals.has("Source")) {
                    this->property_signals.get("Source")->Fire(v);
                }
            }
            return 0;
        }
        fn.error("Expected permission level >=PLUGIN for protected property Source");
    } else 
        return BaseScript::lua_set(L);
}
Script::~Script() {
    setEnable(false, true);
}
void Script::before_start() {
    if (LinkedSource != "[Embedded]" && LinkedSource != "") {
        RuntimeSource = VM->open_script_asset(LinkedSource);
    } else {
        RuntimeSource = Source;
    }
}
void Script::_clone_object(Instance* i) const {
}


};