#ifndef RBLX_INSTANCE
#define RBLX_INSTANCE

#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>

#include "rblx_basic_types.hpp"
#include "rblx_main.hpp"
#include "rblx_events.hpp"

namespace godot {

enum InstanceType {
    T_INSTANCE,
    T_PVINSTANCE,
    T_MODEL,
    T_ACTOR,
    T_LUASOURCECONTAINER,
    T_BASESCRIPT,
    T_DATAMODEL,
    T_SCRIPT,
    T_BASEPART,
    //...
};
struct InstanceClassWriteProxy final {
private:
    struct _InstanceWriteProxy final {
        private:
            Vector<void*> children;
            HashSet<LuaString, LuaStringHasher> tags;
            HashMap<LuaString, RBXVariant, LuaStringHasher> attributes;
            HashMap<LuaString, RBXScriptSignal*, LuaStringHasher> attribute_signals;
            HashMap<LuaString, RBXScriptSignal*, LuaStringHasher> property_signals;
            Vector<int64_t> refs;
            void* parent = nullptr;
            LuaString Name;
            RobloxVMInstance *VM;
            bool parent_locked = false;
            void _clone_object(void*) const {};
            virtual void clone_object() const {}; // TODO: mark this as abstract
            virtual void is_a(const LuaString& s) const {};
            void add_ref(void* i) {};
            void add_ref(RBXScriptSignal* s) {};
            void has_ref(void* i) {};
            void has_ref(RBXScriptSignal* s) {};
            void remove_ref(void* i) {};
            void remove_ref(RBXScriptSignal* i) {};
            int64_t network_id = (size_t)(void*)this;
            virtual ~_InstanceWriteProxy() {};

            bool Archivable = true;
            LuaString getName() const {};
            void setName(LuaString newname) {};
            void* getParent() const {};
            void setParent(void* newparent) {};
        public:
            const char* ClassName;
            InstanceType Type = T_INSTANCE; // Internal
        private:
            virtual void lua_get(lua_State *L) {};
            virtual void lua_set(lua_State *L) {};

            static void lua_static_get(lua_State *L) {};
            static void lua_static_set(lua_State *L) {};
            static void lua_static_tostring(lua_State *L) {};

            // Lua functions
            static void AddTag(lua_State *L) {};
            static void ClearAllChildren(lua_State *L) {};
            static void Clone(lua_State *L) {};
            static void Destroy(lua_State *L) {};
            static void FindFirstAncestor(lua_State *L) {};
            static void FindFirstAncestorOfClass(lua_State *L) {};
            static void FindFirstAncestorWhichIsA(lua_State *L) {};
            static void FindFirstChild(lua_State *L) {};
            static void FindFirstChildOfClass(lua_State *L) {};
            static void FindFirstChildWhichIsA(lua_State *L) {};
            static void FindFirstDescendant(lua_State *L) {};
            static void GetActor(lua_State *L) {};
            static void GetAttribute(lua_State *L) {};
            static void GetAttributeChangedSignal(lua_State *L) {};
            static void GetAttributes(lua_State *L) {};
            static void GetChildren(lua_State *L) {};
            static void GetDebugId(lua_State *L) {};
            static void GetDescendants(lua_State *L) {};
            static void GetFullName(lua_State *L) {};
            static void GetPropertyChangedSignal(lua_State *L) {};
            static void GetTags(lua_State *L) {};
            static void HasTag(lua_State *L) {};
            static void IsA(lua_State *L) {};
            static void IsAncestorOf(lua_State *L) {};
            static void IsDescendantOf(lua_State *L) {};
            static void RemoveTag(lua_State *L) {};
            static void SetAttribute(lua_State *L) {};
            static void WaitForChild(lua_State *L) {};

            void destroy() {};// Internal methods
            void destroy_children() {};// Internal methods

            // SIGNALS
            RBXScriptSignal *AncestryChanged;
            RBXScriptSignal *AttributeChanged;
            RBXScriptSignal *Changed;
            RBXScriptSignal *ChildAdded;
            RBXScriptSignal *ChildRemoved;
            RBXScriptSignal *DescendantAdded;
            RBXScriptSignal *DescendantRemoving;
            RBXScriptSignal *Destroying;

            static void new_instance(lua_State *L) {};
            static void delete_instance(lua_State *L, void *i) {};
    } *obj;
public:
    InstanceClassWriteProxy(Instance* i) {obj = (_InstanceWriteProxy*)(void*)i;}
    _InstanceWriteProxy* operator->() {
        return obj;
    }
};

class Instance {
protected:
    friend class RobloxVMInstance;
    Vector<Instance*> children;
    HashSet<LuaString, LuaStringHasher> tags;
    HashMap<LuaString, RBXVariant, LuaStringHasher> attributes;
    HashMap<LuaString, RBXScriptSignal*, LuaStringHasher> attribute_signals;
    HashMap<LuaString, RBXScriptSignal*, LuaStringHasher> property_signals;
    Vector<int64_t> refs;
    Instance* parent = nullptr;// Properties
    LuaString Name;// Properties
    RobloxVMInstance *VM;
    bool parent_locked = false;
    void _clone_object(Instance*) const;
    virtual Instance* clone_object() const; // TODO: mark this as abstract
    virtual bool is_a(const LuaString& s) const;
    virtual bool is_a(const InstanceType t) const;
    virtual bool has_property(const LuaString& s, bool recurse = true) const;
    int64_t add_ref(Instance* i);
    int64_t add_ref(RBXScriptSignal* s);
    bool has_ref(Instance* i);
    bool has_ref(RBXScriptSignal* s);
    void remove_ref(Instance* i);
    void remove_ref(RBXScriptSignal* i);
public:
    int64_t network_id = (size_t)(void*)this;
    Instance(RobloxVMInstance *VM);
    virtual ~Instance();

    // PROPERTIES
    bool Archivable = true;
    LuaString getName() const;
    void setName(LuaString newname);
    Instance* getParent() const;
    void setParent(Instance* newparent);
    const char* volatile const ClassName = "Instance";
    volatile const InstanceType Type = T_INSTANCE; // Internal

    virtual int lua_get(lua_State *L);
    virtual int lua_set(lua_State *L);

    static int lua_static_get(lua_State *L);
    static int lua_static_set(lua_State *L);
    static int lua_static_tostring(lua_State *L);

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

    void destroy();// Internal methods
    void destroy_children();// Internal methods

    // SIGNALS
    RBXScriptSignal *AncestryChanged;
    RBXScriptSignal *AttributeChanged;
    RBXScriptSignal *Changed;
    RBXScriptSignal *ChildAdded;
    RBXScriptSignal *ChildRemoved;
    RBXScriptSignal *DescendantAdded;
    RBXScriptSignal *DescendantRemoving;
    RBXScriptSignal *Destroying;

    static int new_instance(lua_State *L);
    static void delete_instance(lua_State *L, void *i);
};

}

#endif