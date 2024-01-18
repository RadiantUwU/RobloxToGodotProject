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
    //destroy_children(); // Causes C stack overflow from Luau running GC during destructor
    //Note: This will delete the object without firing any events!
    //TODO: Events will hold a reference to the instance.
    for (int ref : refs) {
        ls.delete_ref(ref);
    }
}
void Instance::destroy() {
    if (parent != nullptr && !destroyed) {
        Destroying->Fire();
        destroy_children();
        parent_locked = false;
        setParent(nullptr);
        parent_locked = true;
        destroyed = true;
    }
}
void Instance::destroy_children() {
    Vector<Instance*> children_clone = children;
    for (Instance* i : children_clone) {
        i->destroy();
    }
}
Instance* Instance::getParent() const {
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
        Instance* ancestor = parent;
        while (ancestor != nullptr) {
            ancestor->DescendantRemoving->Fire(this);
            ancestor = ancestor->parent;
        }
    }
    if (parent) {
        RBLX_PRINT_VERBOSE("Starting recursion through parent's references table");
        for (int64_t ref : parent->refs) {
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
    Instance* oldparent = parent;
    parent = newparent;
    if (newparent) {
        parent->children.append(this);
        RBLX_PRINT_VERBOSE("Creating new reference of parent...");
        luau_context ctx = VM->main_synchronized;
        ctx.rawget(LUA_REGISTRYINDEX,"USERDATA_REFS");
        ctx.rawget(-1,(size_t)(void*)this);
        parent->refs.append(ctx.new_ref(-1));
    }
    if (parent) {
        parent->ChildRemoved->Fire(this);
    }
    this->Changed->Fire("Parent");
    if (this->property_signals.has("Parent")) {
        this->property_signals.get("Parent")->Fire(newparent);
    }
    if (newparent) {
        newparent->ChildAdded->Fire(this);
        Instance *ancestor = newparent;
        while (ancestor != nullptr) {
            ancestor->DescendantAdded->Fire(this);
            ancestor = ancestor->getParent();
        }
    }
    this->AncestryChanged->Fire(this,newparent);
    Vector<Instance*> instances = children;
    Vector<Instance*> awaiting_instances;
    while (instances.size() != 0) {
        for (Instance* descendant : instances) {
            descendant->AncestryChanged->Fire(this, newparent);
            awaiting_instances.append_array(descendant->children);
        }
        instances = awaiting_instances;
        awaiting_instances.clear();
    }

}
LuaString Instance::getName() const {
    return Name;
}
void Instance::setName(LuaString n) {
    Name = n;
    this->Changed->Fire("Name");
    if (this->property_signals.has("Name")) {
        this->property_signals.get("Name")->Fire(n);
    }
}
int64_t Instance::add_ref(Instance *i) {
    luau_context ctx = this->VM->main_synchronized;
    ctx.push_object(i);
    int64_t ref = ctx.new_ref(-1);
    refs.append(ref);
    return ref;
}
int64_t Instance::add_ref(RBXScriptSignal *s) {
    luau_context ctx = this->VM->main_synchronized;
    ctx.push_object(s);
    int64_t ref = ctx.new_ref(-1);
    refs.append(ref);
    return ref;
}
void Instance::add_ref(int64_t ref) {
    refs.append(ref);
}
bool Instance::has_ref(Instance* i) {
    for (int64_t ref : refs) {
        luau_context ctx = VM->main_synchronized;
        ctx.push_ref(ref);
        if (ctx.as_userdata<Instance>(-1) == i) {
            return true;
        }
    }
    return false;
}
bool Instance::has_ref(RBXScriptSignal* s) {
    for (int64_t ref : refs) {
        luau_context ctx = VM->main_synchronized;
        ctx.push_ref(ref);
        if (ctx.as_userdata<RBXScriptSignal>(-1) == s) {
            return true;
        }
    }
    return false;
}
bool Instance::has_ref(int64_t ref) {
    return refs.find(ref) != -1;
}
void Instance::remove_ref(Instance* i) {
    for (int64_t ref : refs) {
        luau_context ctx = VM->main_synchronized;
        ctx.push_ref(ref);
        if (ctx.as_userdata<Instance>(-1) == i) {
            ctx.delete_ref(ref);
            refs.erase(ref);
            break;
        }
    }
}
void Instance::remove_ref(RBXScriptSignal* s) {
    for (int64_t ref : refs) {
        luau_context ctx = VM->main_synchronized;
        ctx.push_ref(ref);
        if (ctx.as_userdata<RBXScriptSignal>(-1) == s) {
            ctx.delete_ref(ref);
            refs.erase(ref);
            break;
        }
    }
}
void Instance::remove_ref(int64_t ref) {
    refs.erase(ref);
}
bool Instance::is_a(const LuaString& s) const {
    return s == "Instance";
}
bool Instance::is_a(const InstanceType t) const {
    return t == T_INSTANCE;
}
bool Instance::has_property(const LuaString& s, bool recurse) const {
    if (s == "Archivable") return true;
    else if (s == "ClassName") return true;
    else if (s == "Name") return true;
    else if (s == "Parent") return true;
    return false;
}

Instance *Instance::clone_object() const {
    luau_context ctx = VM->main_synchronized;
    //TODO: Add defer reference
    //TODO: Associate to actor
    Instance* i = ctx.new_instance<Instance>(VM);
    _clone_object(i);
    return i;
}
void Instance::_clone_object(Instance* i) const {
    i->Archivable = Archivable;
    i->Name = Name;
}

int Instance::AddTag(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(2);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "tag", LUA_TSTRING);
    Instance* i = fn.as_userdata<Instance>(1);
    RBXVariant v = fn.to_object(2);
    LuaString tag = LuaString(v.get_str(),v.get_slen());
    if (!i->tags.has(tag)) {
        i->tags.insert(tag);
        luau_context ctx = i->VM->main_synchronized;
        ctx.getregistry("INSTANCE_TAGS");
        ctx.push_object(tag);
        ctx.rawget(-2);
        if (ctx.is_type(-1, LUA_TNIL)) {
            ctx.pop_stack(1);
            ctx.push_object(tag);
            ctx.new_table();
            ctx.getregistry("WEAKTABLE_K");
            ctx.setmetatable(-2);
            ctx.rawset(-3);
        }
        ctx.push_object(i);
        ctx.push_object(true);
        ctx.rawset(-3);
    }
    return fn.lua_return(0);
}
int Instance::ClearAllChildren(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(1);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    Instance* i = fn.as_userdata<Instance>(1);
    i->destroy_children();
    return fn.lua_return(0);
}
int Instance::Clone(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(1);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    Instance* i = fn.as_userdata<Instance>(1);
    if (!i->Archivable) {
        return 0;
    }// ASSERT: the underlying lua state is the main state
    // TODO: Fix this for multi-threading
    fn.push_object(i->clone_object());
    return fn.lua_return(1);
}
int Instance::Destroy(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(1);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    Instance* i = fn.as_userdata<Instance>(1);
    i->destroy();
    return fn.lua_return(0);
}
int Instance::FindFirstAncestor(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(2);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "name", LUA_TSTRING);
    Instance* i = fn.as_userdata<Instance>(1);
    RBXVariant v = fn.as_object(2);
    LuaString n = LuaString(v.get_str(),v.get_slen());
    Instance* ancestor = i->parent;
    while (ancestor != nullptr) {
        if (ancestor->Name == n) {
            fn.push_object(ancestor);
            return fn.lua_return(1);
        }
        ancestor = ancestor->parent;
    }
    return fn.lua_return(0);
}
int Instance::FindFirstAncestorOfClass(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(2);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "class", LUA_TSTRING);
    Instance* i = fn.as_userdata<Instance>(1);
    RBXVariant v = fn.as_object(2);
    LuaString n = LuaString(v.get_str(),v.get_slen());
    Instance* ancestor = i->parent;
    while (ancestor != nullptr) {
        if (n == ancestor->ClassName) {
            fn.push_object(ancestor);
            return fn.lua_return(1);
        }
        ancestor = ancestor->parent;
    }
    return fn.lua_return(0);
}
int Instance::FindFirstAncestorWhichIsA(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(2);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "class", LUA_TSTRING);
    Instance* i = fn.as_userdata<Instance>(1);
    RBXVariant v = fn.as_object(2);
    LuaString n = LuaString(v.get_str(),v.get_slen());
    Instance* ancestor = i->parent;
    while (ancestor != nullptr) {
        luau_context ctx = L;
        ctx.push_object(Instance::IsA,"Instance::IsA");
        ctx.push_objects(ancestor,v);
        ctx.call(2,1);
        RBXVariant res = ctx.to_object(); //TODO: change this for all functions that use IsA in this file
        if ((bool)res) {
            fn.push_object(ancestor);
            return fn.lua_return(1);
        }
    }
    return fn.lua_return(0);
}
int Instance::FindFirstChild(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(2,1);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "name", LUA_TSTRING);
    Instance* i = fn.as_userdata<Instance>(1);
    RBXVariant v = fn.as_object(2);
    LuaString n = LuaString(v.get_str(),v.get_slen());
    bool recursive = false;
    if (fn.get_stack_size() == 3) {
        fn.assert_type_argument(3,"recursive",LUA_TBOOLEAN);
        v = fn.as_object(3);
        recursive = (bool)v;
    }
    if (recursive) {
        Vector<Instance*> instances = i->children;
        Vector<Instance*> awaiting_instances;
        while (instances.size() != 0) {
            for (Instance* descendant : instances) {
                if (descendant->Name == n) {
                    fn.push_object(descendant);
                    return fn.lua_return(1);
                } else {
                    awaiting_instances.append_array(descendant->children);
                }
            }
            instances = awaiting_instances;
            awaiting_instances.clear();
        }
    } else {
        for (Instance* child : i->children) {
            if (child->Name == n) {
                fn.push_object(child);
                return fn.lua_return(1);
            }
        }
    }
    return fn.lua_return(0);
}
int Instance::FindFirstChildOfClass(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(2);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "class", LUA_TSTRING);
    Instance* i = fn.as_userdata<Instance>(1);
    RBXVariant v = fn.as_object(2);
    LuaString n = LuaString(v.get_str(),v.get_slen());
    for (Instance* child : i->children) {
        if (n == child->ClassName) {
            fn.push_object(child);
            return fn.lua_return(1);
        }
    }
    return fn.lua_return(0);
}
int Instance::FindFirstChildWhichIsA(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(2);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "class", LUA_TSTRING);
    Instance* i = fn.as_userdata<Instance>(1);
    RBXVariant v = fn.as_object(2);
    LuaString n = LuaString(v.get_str(),v.get_slen());
    for (Instance* child : i->children) {
        luau_context ctx = L;
        ctx.push_object(Instance::IsA,"Instance::IsA");
        ctx.push_objects(child,v);
        ctx.call(2,1);
        RBXVariant res = ctx.to_object();
        if ((bool)res) {
            fn.push_object(child);
            return fn.lua_return(1);
        }
    }
    return fn.lua_return(0);
}
int Instance::FindFirstDescendant(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(2);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "name", LUA_TSTRING);
    Instance* i = fn.as_userdata<Instance>(1);
    RBXVariant v = fn.as_object(2);
    LuaString n = LuaString(v.get_str(),v.get_slen());
    Vector<Instance*> instances = i->children;
    Vector<Instance*> awaiting_instances;
    while (instances.size() != 0) {
        for (Instance* descendant : instances) {
            if (descendant->Name == n) {
                fn.push_object(descendant);
                return fn.lua_return(1);
            } else {
                awaiting_instances.append_array(descendant->children);
            }
        }
        instances = awaiting_instances;
        awaiting_instances.clear();
    }
    return fn.lua_return(0);
}
int Instance::GetActor(lua_State *L) {
    return 0; // TODO: Implement Actor instances
}
int Instance::GetAttribute(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(2);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "attribute", LUA_TSTRING);
    Instance* i = fn.as_userdata<Instance>(1);
    RBXVariant v = fn.as_object(2);
    LuaString attr = LuaString(v.get_str(),v.get_slen());
    if (i->attributes.has(attr)) {
        fn.push_object(i->attributes.get(attr));
        return fn.lua_return(1);
    }
    return fn.lua_return(0);
}
int Instance::GetAttributeChangedSignal(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(2);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "attribute", LUA_TSTRING);
    Instance* i = fn.as_userdata<Instance>(1);
    RBXVariant v = fn.as_object(2);
    LuaString attr = LuaString(v.get_str(),v.get_slen());
    if (i->attribute_signals.has(attr)) {
        fn.push_object(i->attribute_signals.get(attr));
        return fn.lua_return(1);
    } else {
        RBXScriptSignal *sig = fn.new_userdata<RBXScriptSignal>(fn.UD_TRBXSCRIPTSIGNAL,L);
        fn.push_value(-1);
        i->refs.append(fn.new_ref(-1));
        i->attribute_signals.insert(attr,sig);
        return fn.lua_return(1);
    }
}
int Instance::GetAttributes(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(1);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    Instance* i = fn.as_userdata<Instance>(1);
    fn.new_dictionary(i->attributes.size());
    for (KeyValue<LuaString, RBXVariant>& attrib_pair : i->attributes) {
        fn.push_objects(attrib_pair.key, attrib_pair.value);
        fn.rawset(-3);
    }
    return fn.lua_return(1);
}
int Instance::GetChildren(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(1);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    Instance* i = fn.as_userdata<Instance>(1);
    fn.new_array(i->children.size());
    int index = 1;
    for (Instance* child : i->children) {
        fn.push_object(child);
        fn.rawset(-2,index++);
    }
    return fn.lua_return(1);
}
int Instance::GetDebugId(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(1);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    Instance* i = fn.as_userdata<Instance>(1);
    fn.push_object(i->network_id);
    return fn.lua_return(1);
}
int Instance::GetDescendants(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(1);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    Instance* i = fn.as_userdata<Instance>(1);
    Vector<Instance*> instances = i->children;
    Vector<Instance*> await_instances;
    fn.new_table();
    int index = 1;
    while (instances.size() == 0) {
        for (Instance* descendant : instances) {
            fn.push_object(descendant);
            fn.rawset(-2,index++);
            await_instances.append_array(descendant->children);
        }
        instances = await_instances;
        await_instances.clear();
    }
    return fn.lua_return(1);
}
int Instance::GetFullName(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(1);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    Instance* i = fn.as_userdata<Instance>(1);
    Vector<LuaString> names = {i->Name};
    Instance* ancestor = i->parent;
    bool final_game = false;
    while (ancestor != nullptr) {
        if (ancestor->Type == T_DATAMODEL) {
            final_game = true;
            break;
        }
        names.append(ancestor->Name);
        ancestor = ancestor->parent;
    }
    names.reverse();
    int size,nsize = 0;
    for (LuaString& n : names) {
        size += n.l+1;
    }
    nsize = names.size()-1;
    LuaString fullname((final_game) ? size+5 : size);
    size_t start = 0;
    if (final_game) {
        start = 5*sizeof(char);
        memcpy(fullname.s,"game.",sizeof(char)*5);
    }
    for (LuaString& n : names) {
        memcpy(fullname.s+start,n.s,n.l*sizeof(char));
        start += n.l*sizeof(char);
        if (nsize-- != 0) {
            *(fullname.s+start) = '.';
            start += sizeof(char);
        }
    }
    fn.push_object(fullname);
    return fn.lua_return(1);

}
int Instance::GetPropertyChangedSignal(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(2);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "property", LUA_TSTRING);
    Instance* i = fn.as_userdata<Instance>(1);
    RBXVariant v = fn.as_object(2);
    LuaString property = LuaString(v.get_str(),v.get_slen());
    if (i->property_signals.has(property)) {
        fn.push_object(i->property_signals.get(property));
        return fn.lua_return(1);
    } else if (i->has_property(property)) {
        RBXScriptSignal *sig = fn.new_userdata<RBXScriptSignal>(fn.UD_TRBXSCRIPTSIGNAL,L);
        fn.push_value(-1);
        i->refs.append(fn.new_ref(-1));
        i->property_signals.insert(property,sig);
        return fn.lua_return(1);
    } else {
        fn.errorf("property %s does not exist in type %s",property.s,i->ClassName);
    }
}
int Instance::GetTags(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(1);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    Instance* i = fn.as_userdata<Instance>(1);
    int index = 1;
    fn.new_array(i->tags.size());
    for (const LuaString& str : i->tags) {
        fn.push_object(str);
        fn.rawset(-2,index++);
    }
    return fn.lua_return(1);
}
int Instance::HasTag(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(2);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "tag", LUA_TSTRING);
    Instance* i = fn.as_userdata<Instance>(1);
    RBXVariant v = fn.as_object(2);
    LuaString tag = LuaString(v.get_str(),v.get_slen());
    fn.push_object(i->tags.has(tag));
    return fn.lua_return(1);
}
int Instance::IsA(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(2);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "class", LUA_TSTRING);
    Instance* i = fn.as_userdata<Instance>(1);
    RBXVariant v = fn.as_object(2);
    LuaString s = LuaString(v.get_str(),v.get_slen());
    fn.push_object(i->is_a(s));
    return fn.lua_return(1);
}
int Instance::IsAncestorOf(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(2);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_usertype_argument(2, "ancestor", fn.UD_TINSTANCE);
    Instance* self = fn.as_userdata<Instance>(1);
    Instance* descendant = fn.as_userdata<Instance>(2);
    Instance* ancestor = descendant->parent;
    while (ancestor != nullptr) {
        if (ancestor == self) {
            fn.push_object(true); return fn.lua_return(1);
        }
        ancestor = ancestor->parent;
    }
    fn.push_object(false); return fn.lua_return(1);
}
int Instance::IsDescendantOf(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(2);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_usertype_argument(2, "descendant", fn.UD_TINSTANCE);
    Instance* self = fn.as_userdata<Instance>(1);
    Instance* other = fn.as_userdata<Instance>(2);
    Instance* ancestor = self->parent;
    while (ancestor != nullptr) {
        if (ancestor == other) {
            fn.push_object(true); return fn.lua_return(1);
        }
        ancestor = ancestor->parent;
    }
    fn.push_object(false); return fn.lua_return(1);
}
int Instance::RemoveTag(lua_State *L) { // TODO: Implement CollectionService and events
    luau_function_context fn = L;
    fn.assert_stack_size(2);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "tag", LUA_TSTRING);
    Instance* i = fn.as_userdata<Instance>(1);
    RBXVariant v = fn.as_object(2);
    LuaString tag = LuaString(v.get_str(),v.get_slen());
    if (i->tags.has(tag)) {
        i->tags.erase(tag);
    }
    return fn.lua_return(0);
}
int Instance::SetAttribute(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_stack_size(2,1);
    fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE);
    fn.assert_type_argument(2, "attribute", LUA_TSTRING);
    Instance* i = fn.as_userdata<Instance>(1);
    RBXVariant v = fn.as_object(2);
    LuaString attrib = LuaString(v.get_str(),v.get_slen());
    if (fn.get_stack_size() != 2) {
        if (fn.get_userdata_type(3) == fn.UD_TINSTANCE)
            fn.error("Cannot store instances as attributes, if this is an incompatibility open an issue.");
    } else {
        fn.push_object();
    }
    v = fn.as_object(3);
    if (((v.type == RBXVariant::Type::RBXVARIANT_NIL) xor i->attributes.has(attrib)) or i->attributes.get(attrib) != v) {
        i->attributes[attrib] = v;
        i->AttributeChanged->Fire(attrib);
        if (i->attribute_signals.has(attrib)) {
            i->attribute_signals.get(attrib)->Fire(v);
        }
    }
    return fn.lua_return(0);
}
int Instance::WaitForChild(lua_State *L) {
    return 0; // TODO: Add task scheduler for waitforchild and make a continuation function
}

int Instance::lua_get(lua_State *L) {
    luau_function_context fn = L;
    //fn.assert_usertype_argument(1, "self", fn.UD_TINSTANCE); // handled by lua_static_get
    //fn.assert_type_argument(2, "key", LUA_TSTRING);
    RBXVariant v = fn.as_object(2);
    LuaString s = LuaString(v.get_str(),v.get_slen()); // TODO: Optimize with a hash map. I dont want this to become yandere dev's masterpiece
    if (s == "Archivable") {
        fn.push_object(this->Archivable);
        return fn.lua_return(1);
    } else if (s == "ClassName") {
        fn.push_object((const char*)this->ClassName);
        return fn.lua_return(1);
    } else if (s == "Name") {
        fn.push_object(this->Name.s,(size_t)this->Name.l);
        return fn.lua_return(1);
    } else if (s == "Parent") {
        if (parent == nullptr) {
            fn.push_object();
        } else {
            fn.rawget(LUA_REGISTRYINDEX,"USERDATA_REFS");
            fn.rawget(-1,(int64_t)(void*)parent);
        }
        return fn.lua_return(1);
    } else if (s == "AncestryChanged") {
        fn.push_object(AncestryChanged);
        return fn.lua_return(1);
    } else if (s == "AttributeChanged") {
        fn.push_object(AttributeChanged);
        return fn.lua_return(1);
    } else if (s == "Changed") {
        fn.push_object(Changed);
        return fn.lua_return(1);
    } else if (s == "ChildAdded") {
        fn.push_object(ChildAdded);
        return fn.lua_return(1);
    } else if (s == "ChildRemoved") {
        fn.push_object(ChildRemoved);
        return fn.lua_return(1);
    } else if (s == "DescendantAdded") {
        fn.push_object(DescendantAdded);
        return fn.lua_return(1);
    } else if (s == "DescendantRemoving") {
        fn.push_object(DescendantRemoving);
        return fn.lua_return(1);
    } else if (s == "Destroying") {
        fn.push_object(Destroying);
        return fn.lua_return(1);
    } else if (s == "AddTag") {
        fn.push_object(&Instance::AddTag,"Instance::AddTag");
        return fn.lua_return(1);
    } else if (s == "ClearAllChildren") {
        fn.push_object(&Instance::ClearAllChildren,"Instance::ClearAllChildren");
        return fn.lua_return(1);
    } else if (s == "Clone") {
        fn.push_object(&Instance::Clone,"Instance::Clone");
        return fn.lua_return(1);
    } else if (s == "Destroy") {
        fn.push_object(&Instance::Destroy,"Instance::Destroy");
        return fn.lua_return(1);
    } else if (s == "FindFirstAncestor") {
        fn.push_object(&Instance::FindFirstAncestor,"Instance::FindFirstAncestor");
        return fn.lua_return(1);
    } else if (s == "FindFirstAncestorOfClass") {
        fn.push_object(&Instance::FindFirstAncestorOfClass,"Instance::FindFirstAncestorOfClass");
        return fn.lua_return(1);
    } else if (s == "FindFirstAncestorWhichIsA") {
        fn.push_object(&Instance::FindFirstAncestorWhichIsA,"Instance::FindFirstAncestorWhichIsA");
        return fn.lua_return(1);
    } else if (s == "FindFirstChild") {
        fn.push_object(&Instance::FindFirstChild,"Instance::FindFirstChild");
        return fn.lua_return(1);
    } else if (s == "FindFirstChildOfClass") {
        fn.push_object(&Instance::FindFirstChildOfClass,"Instance::FindFirstChildOfClass");
        return fn.lua_return(1);
    } else if (s == "FindFirstChildWhichIsA") {
        fn.push_object(&Instance::FindFirstChildWhichIsA,"Instance::FindFirstChildWhichIsA");
        return fn.lua_return(1);
    } else if (s == "FindFirstDescendant") {
        fn.push_object(&Instance::FindFirstDescendant,"Instance::FindFirstDescendant");
        return fn.lua_return(1);
    } else if (s == "GetActor") {
        fn.push_object(&Instance::GetActor,"Instance::GetActor");
        return fn.lua_return(1);
    } else if (s == "GetAttribute") {
        fn.push_object(&Instance::GetAttribute,"Instance::GetAttribute");
        return fn.lua_return(1);
    } else if (s == "GetAttributeChangedSignal") {
        fn.push_object(&Instance::GetAttributeChangedSignal,"Instance::GetAttributeChangedSignal");
        return fn.lua_return(1);
    } else if (s == "GetAttributes") {
        fn.push_object(&Instance::GetAttributes,"Instance::GetAttributes");
        return fn.lua_return(1);
    } else if (s == "GetChildren") {
        fn.push_object(&Instance::GetChildren,"Instance::GetChildren");
        return fn.lua_return(1);
    } else if (s == "GetDebugId") {
        fn.push_object(&Instance::GetDebugId,"Instance::GetDebugId");
        return fn.lua_return(1);
    } else if (s == "GetDescendants") {
        fn.push_object(&Instance::GetDescendants,"Instance::GetDescendants");
        return fn.lua_return(1);
    } else if (s == "GetFullName") {
        fn.push_object(&Instance::GetFullName,"Instance::GetFullName");
        return fn.lua_return(1);
    } else if (s == "GetPropertyChangedSignal") {
        fn.push_object(&Instance::GetPropertyChangedSignal,"Instance::GetPropertyChangedSignal");
        return fn.lua_return(1);
    } else if (s == "GetTags") {
        fn.push_object(&Instance::GetTags,"Instance::GetTags");
        return fn.lua_return(1);
    } else if (s == "HasTag") {
        fn.push_object(&Instance::HasTag,"Instance::HasTag");
        return fn.lua_return(1);
    } else if (s == "IsA") {
        fn.push_object(&Instance::IsA,"Instance::IsA");
        return fn.lua_return(1);
    } else if (s == "IsAncestorOf") {
        fn.push_object(&Instance::IsAncestorOf,"Instance::IsAncestorOf");
        return fn.lua_return(1);
    } else if (s == "IsDescendantOf") {
        fn.push_object(&Instance::IsDescendantOf,"Instance::IsDescendantOf");
        return fn.lua_return(1);
    } else if (s == "RemoveTag") {
        fn.push_object(&Instance::RemoveTag,"Instance::RemoveTag");
        return fn.lua_return(1);
    } else if (s == "SetAttribute") {
        fn.push_object(&Instance::SetAttribute,"Instance::SetAttribute");
        return fn.lua_return(1);
    } else if (s == "WaitForChild") {
        fn.push_object(&Instance::WaitForChild,"Instance::WaitForChild");
        return fn.lua_return(1);
    } else {
        for (Instance *child : this->children) {
            if (child->Name == s) {
                fn.rawget(LUA_REGISTRYINDEX,"USERDATA_REFS");
                fn.rawget(-1,(int64_t)(void*)child);
                return fn.lua_return(1);
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
        bool b = (bool)fn.as_object(3);
        bool should_fire = this->Archivable != b;
        this->Archivable = b;
        if (should_fire) {
            this->Changed->Fire("Archivable");
            if (this->property_signals.has("Archivable")) {
                this->property_signals.get("Archivable")->Fire(b);
            }
        }
        return fn.lua_return(0);
    } else if (strcmp(s,"ClassName") == 0) {
        fn.error("cannot set read only property ClassName");
    } else if (strcmp(s,"Name") == 0) {
        fn.assert_type_argument(3, "value", LUA_TSTRING);
        RBXVariant v = fn.as_object(3);
        setName(LuaString(v.get_str(),v.get_slen()));
        return fn.lua_return(0);
    } else if (strcmp(s,"Parent") == 0) {
        Instance *newparent;
        if (fn.is_type(3, LUA_TNIL)) {
            newparent = nullptr;
        } else {
            fn.assert_usertype_argument(3, "value", fn.UD_TINSTANCE);
            newparent = fn.as_userdata<Instance>(3);
        }
        this->setParent(newparent);
        return fn.lua_return(0);
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
    return fn.lua_return(1);
}
}