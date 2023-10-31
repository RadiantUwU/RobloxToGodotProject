#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <lua.h>
#include <lualib.h>
#include <luacode.h>

#include <classes/luau_function.h>
#include <utils.h>
#include <vector_lib.h>
#include "rblx_main.hpp"


#define RobloxToGodotProject_REGISTRY_NODE_KEY "RobloxToGodotProject_node"


namespace godot {

class LuauVM : public Node {
    GDCLASS(LuauVM, Node)
    RobloxVMInstance* vm;
private:
    lua_State* L;
    void create_metatables();

    static int task_create(lua_State *L);
    static int task_defer(lua_State *L);
    static int task_delay(lua_State *L);
    static int task_desynchronize(lua_State *L);
    static int task_synchronize(lua_State *L);
    static int task_wait(lua_State *L);
    static int task_cancel(lua_State *L);

    void handle_error(lua_State *thread);
    void terminate_error(lua_State *thread);
    bool task_resumption_cycle(bool terminate = false);
protected:
    static void _bind_methods();
    static void _bind_passthrough_methods();

public:
    LuauVM();
    ~LuauVM();

    void register_print();

    int load_string(const String &code, const String &chunkname);
    int do_string(const String &code, const String &chunkname);

    void open_libraries(const Array &libraries);
    void open_all_libraries();

    int64_t get_memory_usage_bytes();


    // Bindings

    #pragma region RobloxToGodotProject

    void lua_pushvariant(const Variant &var);
    void lua_pusharray(const Array &array);
    void lua_pushdictionary(const Dictionary &dictionary);

    Variant lua_tovariant(int index);
    Array lua_toarray(int index);
    Dictionary lua_todictionary(int index);
    
    Ref<LuauFunction> lua_tofunction(int index);
    Error lua_pushcallable(const Callable &callable);

    void lua_pushobject(Object *node);
    Object *lua_toobject(int idx);

    #pragma endregion

    #pragma region Default

    String (luaL_tostring)(int index);

    #pragma endregion

    #pragma region Auxiliary

    bool luaL_hasmetatable(int index, const String &tname);

    void(luaL_error)(const String &err);
    bool luaL_callmeta(int obj, const String &field);
    bool luaL_getmetafield(int obj, const String &field);
    bool luaL_newmetatable(const String &tname);
    bool (luaL_getmetatable)(const String &tname);
    void luaL_where(int lvl);
    void luaL_typerror(int nargs, const String &tname);
    void (luaL_argcheck)(bool cond, int narg, const String &msg);

    void luaL_checkany(int narg);
    int luaL_checkint(int narg);
    double luaL_checknumber(int narg);
    String(luaL_checkstring)(int narg);
    Vector3 luaL_checkvector(int narg);
    Object *luaL_checkobject(int narg, bool valid);
    void luaL_checktype(int narg, int type);
    void luaL_checkstack(int sz, const String &messsage);
    int luaL_checkoption(int narg, const Array &array, const String &def);

    #pragma endregion
};
}

void lua_setnode(lua_State* L, godot::LuauVM* node);
godot::LuauVM* lua_getnode(lua_State* L);

VARIANT_ENUM_CAST(lua_Status);
VARIANT_ENUM_CAST(lua_Type);
VARIANT_ENUM_CAST(lua_GCOp);

int luaopen_vector(lua_State *L);

