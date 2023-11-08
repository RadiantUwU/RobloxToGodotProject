#include "rblx_basic_types.hpp"
#include "rblx_main.hpp"
#include "rblx_instance.hpp"

#ifndef RBLX_BASE_SCRIPT
#define RBLX_BASE_SCRIPT
namespace godot {

class LuaSourceContainer : public Instance {
protected:
    Instance* CurrentEditor = nullptr;
    bool has_property(const LuaString& s, bool recurse = true) const override;
public:
    LuaString RuntimeSource;
    virtual void reload() = 0;

    int lua_get(lua_State *L) override;
    int lua_set(lua_State *L) override;
};

class BaseScript : public LuaSourceContainer {
protected:
    bool Enabled = false;
    int64_t env_ref;
    Instance* actor = nullptr;
    bool has_property(const LuaString& s, bool recurse = true) const override;
    void _clone_object(Instance*)
public:
    LuaString LinkedSource;
    RBLX_RunContext RunContext;
    void reload() override;

    int lua_get(lua_State *L) override;
    int lua_set(lua_State *L) override;

    bool isEnabled() const;
    void setEnable(bool enable);
};

class Script : public BaseScript {
protected:
    bool has_property(const LuaString& s, bool recurse = true) const override;
public:
    Script(RobloxVMInstance* VM);
    ~Script();
    LuaString Source;

    int lua_get(lua_State *L) override;
    int lua_set(lua_State *L) override;

    bool is_a(const LuaString& s) const override;
    bool is_a(const InstanceType t) const override;
    
};

class LocalScript : public Script {
    LocalScript(RobloxVMInstance* VM);
    ~LocalScript();
    bool is_a(const LuaString& s) const override;
    bool is_a(const InstanceType t) const override;
}

}
#endif