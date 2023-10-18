#ifndef RBLX_MAIN
#define RBLX_MAIN

#include <godot_cpp/templates/vector.hpp>
#include <lua.h>
#include <lualib.h>

class Instance;

class RobloxVMInstance {
public:
    lua_State* main_synchronized;
    Instance* game;
    
};

#endif