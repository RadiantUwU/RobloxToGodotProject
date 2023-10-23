#ifndef RBLX_INSTANCE
#define RBLX_INSTANCE

#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/templates/hash_map.hpp>

#include "rblx_main.hpp"
#include "rblx_events.hpp"

namespace godot {

class Instance {
protected:
    friend class RobloxVMInstance;
    Vector<Instance*> children;
    Vector<String> tags;
    HashMap<String, RBXScriptSignal> attribute_signals;
    HashMap<String, RBXScriptSignal> property_signals;
    Instance* parent;
    String Name;
    int network_id;
    int ref = LUA_NOREF;
    RobloxVMInstance *VM;
public:
    Instance(RobloxVMInstance *VM);
    virtual ~Instance();

    // PROPERTIES
    bool Archivable = true;
    String getName();
    void setName(String newname);
    Instance* getParent();
    void setParent(Instance* newparent);
    volatile const char* const ClassName = "Instance";

    virtual bool lua_matching_function_signature(const char* name);
    virtual int lua_get(lua_State *L);
    virtual int lua_set(lua_State *L);

    // Lua functions
    static int AddTag(lua_State *L);
    static int ClearAllChildren(lua_State *L);
    static int Clone(lua_State *L);
    static int Destroy(lua_State *L);
    static int FindFirstAncestor(lua_State *L);
    static int FindFirstAncestorOfClass(lua_State *L);
    static int FindFirstAncestorWhichIsA(lua_State *L);
    static int FindFirstChild(lua_State *L);
    static int FindFirstChildOfClass(lua_State *L);
    static int FindFirstChildWhichIsA(lua_State *L);
    static int FindFirstDescendant(lua_State *L);
    static int GetActor(lua_State *L);
    static int GetAttribute(lua_State *L);
    static int GetAttributeChangedSignal(lua_State *L);
    static int GetAttributes(lua_State *L);
    static int GetChildren(lua_State *L);
    static int GetDebugId(lua_State *L);
    static int GetDescendants(lua_State *L);
    static int GetFullName(lua_State *L);
    static int GetPropertyChangedSignal(lua_State *L);
    static int GetTags(lua_State *L);
    static int HasTag(lua_State *L);
    static int IsA(lua_State *L);
    static int IsAncestorOf(lua_State *L);
    static int IsDescendantOf(lua_State *L);
    static int RemoveTag(lua_State *L);
    static int SetAttribute(lua_State *L);
    static int WaitForChild(lua_State *L);

    // SIGNALS
    RBXScriptSignal *AncestryChanged;
    RBXScriptSignal *AttributeChanged;
    RBXScriptSignal *Changed;
    RBXScriptSignal *ChildAdded;
    RBXScriptSignal *ChildRemoved;
    RBXScriptSignal *DescendantAdded;
    RBXScriptSignal *DescendantRemoving;
    RBXScriptSignal *Destroying;

};

}

#endif