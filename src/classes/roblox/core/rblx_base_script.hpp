#include "rblx_basic_types.hpp"
#include "rblx_main.hpp"
#include "rblx_instance.hpp"

#ifndef RBLX_BASE_SCRIPT
#define RBLX_BASE_SCRIPT
namespace godot {

class LuaSourceContainer : public Instance {
protected:
    Instance* CurrentEditor;
public:
    LuaString RuntimeSource;
    LuaSourceContainer(RobloxVMInstance* vm);
    ~LuaSourceContainer();

    int lua_get(lua_State *L) override;
    int lua_set(lua_State *L) override;

    Instance* getCurrentEditor() const;
    void setCurrentEditor(Instance* i);
};

class BaseScript : public LuaSourceContainer {
protected:
    bool Enabled = false;
public:
    LuaString LinkedSource;
    RBLX_RunContext RunContext;

    int lua_get(lua_State *L) override;
    int lua_set(lua_State *L) override;

    bool isEnabled() const;
    void setEnable(bool enable);
}

}
#endif