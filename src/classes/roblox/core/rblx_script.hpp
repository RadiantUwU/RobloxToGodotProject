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
    LuaSourceContainer(RobloxVMInstance *vm) : Instance(vm) {}
    LuaString RuntimeSource;
    virtual void reload() = 0;

    int lua_get(lua_State *L) override;
    int lua_set(lua_State *L) override;
};

enum RBLX_RunContext {
    Legacy,
    Server,
    Plugin,
    Client
};

class BaseScript : public LuaSourceContainer {
protected:
    bool Enabled = false;
    int64_t env_ref;
    int64_t threads_ref;
    int64_t connections_ref;
    //TODO: potential SIGSEGV when the script variable is removed from the environment and the script is removed from the instance tree without destroying, resulting in no references and a deletion.
    Instance* actor = nullptr;
    bool has_property(const LuaString& s, bool recurse = true) const override;
    void _clone_object(Instance*) const;
    virtual void before_start();
public:
    BaseScript(RobloxVMInstance *vm) : LuaSourceContainer(vm) {}
    LuaString LinkedSource = "[Embedded]";
    RBLX_RunContext RunContext;
    void reload() override;

    int lua_get(lua_State *L) override;
    int lua_set(lua_State *L) override;

    bool isEnabled() const;
    void setEnable(bool enable, bool now = false);
};

class Script : public BaseScript {
protected:
    bool has_property(const LuaString& s, bool recurse = true) const override;
    void _clone_object(Instance*) const;
    Instance* clone_object() const override;
    void before_start() override;
public:
    Script(RobloxVMInstance* vm) : BaseScript(vm) {}
    ~Script();
    LuaString Source;

    int lua_get(lua_State *L) override;
    int lua_set(lua_State *L) override;

    bool is_a(const LuaString& s) const override;
    bool is_a(const InstanceType t) const override;
    
};

class LocalScript : public Script {
protected:
    Instance* clone_object() const override;
public:
    LocalScript(RobloxVMInstance* vm) : Script(vm) {}
    bool is_a(const LuaString& s) const override;
    bool is_a(const InstanceType t) const override;
};

}
#endif