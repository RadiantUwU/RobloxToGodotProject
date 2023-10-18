#include <godot_cpp/variant/builtin_types.hpp>
#include <godot_cpp/variant/variant.hpp>

#ifndef RBLX_INSTANCE
#define RBLX_INSTANCE

class Instance {
protected:
    std::vector<Instance*> children;
    Instance* parent;
public:
    Instance();
    virtual ~Instance();

    // PROPERTIES
    String Name;
    bool Archivable = true;
    volatile const char* const ClassName = "Instance";

    virtual bool lua_matching_function_signature(const char* name);
    virtual int lua_get(lua_State *L);
    virtual int lua_set(lua_State *L);

    // METHODS
    void AddTag(String tag);
    void ClearAllChildren();
    void Clone();
    void Destroy();
    Instance* FindFirstAncestor(String name);
    Instance* FindFirstAncestorOfClass(String className);
    Instance* FindFirstAncestorOfWhichIsA(String className);
    Instance* FindFirstChild(String name, bool recursive = false);
    Instance* FindFirstChildOfClass(String className);
    Instance* FindFirstChildOfWhichIsA(String className);
    Instance* FindFirstDescendant(String name);
    Instance* GetActor(); // return null, not implemented yet
    Variant GetAttribute(String name);
    
};
#endif