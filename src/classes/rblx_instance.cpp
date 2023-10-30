#include <godot_cpp/templates/vector.hpp>
#include <lua.h>
#include <cstddef>
#include <cstring>

#include "rblx_basic_types.hpp"
#include "rblx_events.hpp"
#include "rblx_instance.hpp"
#include "rblx_main.hpp"
#include "rblx_debug.hpp"

namespace godot {

Instance::Instance(RobloxVMInstance *VM) {
    this->VM = VM;
    luau_State *L = VM->main_synchronized;
    luau_context ls = VM->main_synchronized;
    AncestryChanged = ls.new_userdata<RBXScriptSignal>(ls.UD_TRBXSCRIPTSIGNAL, L);
    RBLX_PRINT_VERBOSE("UTYPE: ",ls.get_userdata_type(-1));
    RBLX_PRINT_VERBOSE("expected: ",ls.UD_TRBXSCRIPTSIGNAL);
    refs.append(ls.new_ref(-1));
    AttributeChanged = ls.new_userdata<RBXScriptSignal>(ls.UD_TRBXSCRIPTSIGNAL, L);
    refs.append(ls.new_ref(-1));
    Changed = ls.new_userdata<RBXScriptSignal>(ls.UD_TRBXSCRIPTSIGNAL, L);
    refs.append(ls.new_ref(-1));
    ChildAdded = ls.new_userdata<RBXScriptSignal>(ls.UD_TRBXSCRIPTSIGNAL, L);
    refs.append(ls.new_ref(-1));
    ChildRemoved = ls.new_userdata<RBXScriptSignal>(ls.UD_TRBXSCRIPTSIGNAL, L);
    refs.append(ls.new_ref(-1));
    DescendantAdded = ls.new_userdata<RBXScriptSignal>(ls.UD_TRBXSCRIPTSIGNAL, L);
    refs.append(ls.new_ref(-1));
    DescendantRemoving = ls.new_userdata<RBXScriptSignal>(ls.UD_TRBXSCRIPTSIGNAL, L);
    refs.append(ls.new_ref(-1));
    Destroying = ls.new_userdata<RBXScriptSignal>(ls.UD_TRBXSCRIPTSIGNAL, L);
    refs.append(ls.new_ref(-1));
}
Instance::~Instance() {
    luau_context ls = this->VM->main_synchronized;
    destroy_children();
    for (int ref : refs) {
        ls.delete_ref(ref);
    }
}
void Instance::destroy() {
    if (parent != nullptr) {
        Destroying->Fire();
        destroy_children();
        parent_locked = false;
        setParent(nullptr);
        parent_locked = true;
    }
}
void Instance::destroy_children() {
    Vector<Instance*> children_clone = children;
    for (Instance* i : children_clone) {
        i->destroy();
    }
}
Instance* Instance::getParent() {
    return parent;
}
void Instance::setParent(Instance *newparent) {
    if (newparent == parent) return;
    if (newparent == this) {
        return; // TODO: Log a warning
    }
    if (parent_locked) {
        return; // TODO: Log a warning
    }
    if (parent) {
        RBLX_PRINT_VERBOSE("Starting recursion through parent's references table");
        for (int ref : parent->refs) {
            luau_context ctx = VM->main_synchronized;
            ctx.push_ref(ref);
            if (ctx.as_userdata<Instance>(-1) == this) {
                RBLX_PRINT_VERBOSE("Found reference of old parent: ", ref);
                ctx.delete_ref(ref);
                parent->refs.erase(ref);
                break;
            }
        }
        RBLX_PRINT_VERBOSE("Deleting from parent->children...");
        parent->children.erase(this);
        RBLX_PRINT_VERBOSE("Parent unparented :3");
    }
    parent = newparent;
    if (newparent) {
        parent->children.append(this);
        RBLX_PRINT_VERBOSE("Creating new reference of parent...");
        luau_context ctx = VM->main_synchronized;
        ctx.rawget(LUA_REGISTRYINDEX,"INSTANCE_REFS");
        ctx.rawget(-1,(size_t)(void*)this);
        parent->refs.append(ctx.new_ref(-1));
    }
}
LuaString Instance::getName() {
    return Name;
}
void Instance::setName(LuaString n) {
    Name = n;
}
int Instance::lua_get(lua_State *L) {
    luau_function_context fn = L;
    //fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE); // handled by lua_static_get
    //fn.assert_type_argument(2, "key", LUA_TSTRING);
    RBXVariant v = fn.as_object(2);
    const char *s = v.get_str(); int l = v.get_slen();
    if (strcmp(s,"Archivable") == 0) {
        fn.push_object(this->Archivable);
        return 1;
    } else if (strcmp(s,"ClassName") == 0) {
        fn.push_object(this->ClassName);
        return 1;
    } else if (strcmp(s,"Name") == 0) {
        fn.push_object(this->Name.s,(size_t)this->Name.l);
        return 1;
    } else if (strcmp(s,"Parent") == 0) {
        if (parent == nullptr) {
            fn.push_object();
        } else {
            fn.rawget(LUA_REGISTRYINDEX,"INSTANCE_REFS");
            fn.rawget(-1,(int64_t)(void*)parent);
        }
        return 1;
    } else {
        for (Instance *child : this->children) {
            if (child->Name.l == l && memcmp(child->Name.s,s,l) == 0) {
                fn.rawget(LUA_REGISTRYINDEX,"INSTANCE_REFS");
                fn.rawget(-1,(int64_t)(void*)child);
                return 1;
            }
        }
    }
    fn.errorf("property '%s' not found in type Instance", s);
}
int Instance::lua_set(lua_State *L) {
    luau_function_context fn = L;
    //fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE); // handled by lua_static_get
    //fn.assert_type_argument(2, "key", LUA_TSTRING);
    RBXVariant v = fn.as_object(2);
    const char *s = v.get_str();
    if (strcmp(s,"Archivable") == 0) {
        fn.assert_type_argument(3, "value", LUA_TBOOLEAN);
        this->Archivable = (bool)fn.as_object(3);
        return 0;
    } else if (strcmp(s,"ClassName") == 0) {
        fn.error("cannot set read only property ClassName");
    } else if (strcmp(s,"Name") == 0) {
        fn.assert_type_argument(3, "value", LUA_TSTRING);
        RBXVariant v = fn.as_object(3);
        this->Name = LuaString(v.get_str(),v.get_slen());
        return 0;
    } else if (strcmp(s,"Parent") == 0) {
        Instance *newparent;
        if (fn.is_type(3, LUA_TNIL)) {
            newparent = nullptr;
        } else {
            fn.assert_usertype_argument(3, "value", fn.UD_TINSTANCE);
            newparent = fn.as_userdata<Instance>(3);
        }
        this->setParent(newparent);
        return 0;
    }
    fn.errorf("cannot set inexistent property '%s' in type Instance", s);
}
int Instance::lua_static_get(lua_State *L) {
    luau_function_context fn = L;
    RBLX_PRINT_VERBOSE("Instance::__index");
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "key", LUA_TSTRING);
    Instance *i = fn.as_userdata<Instance>(1);
    return i->lua_get(L);
}
int Instance::lua_static_set(lua_State *L) {
    luau_function_context fn = L;
    RBLX_PRINT_VERBOSE("Instance::__newindex");
    fn.getmetatable(-1);
    auto mt = fn.as_pointer_hash(-1);
    fn.pop_stack(1);
    RBLX_PRINT_VERBOSE(mt);
    RBLX_PRINT_VERBOSE("IS USERDATA self: ",fn.get_type(1)==LUA_TUSERDATA," IS INSTANCE: ",fn.get_userdata_type(1)==fn.UD_TINSTANCE);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "key", LUA_TSTRING);
    Instance *i = fn.as_userdata<Instance>(1);
    return i->lua_set(L);
}
void Instance::delete_instance(lua_State *L, void *i) {
    ((Instance*)i)->~Instance();
}
int Instance::lua_static_tostring(lua_State *L) {
    luau_function_context fn = L;
    RBLX_PRINT_VERBOSE("Instance::__tostring");
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    Instance *i = fn.as_userdata<Instance>(1);
    RBXVariant v = RBXVariant(i->Name.s,i->Name.l);
    fn.push_object(v);
    return 1;
}
}