#ifndef RBLX_MEDIUM_LEVEL_LUAU
#define RBLX_MEDIUM_LEVEL_LUAU

#include <godot_cpp/templates/list.hpp>

#include "rblx_main.hpp"

namespace godot {

class LuaObject {
    friend class luau_context;
    friend class List<LuaObject>;
    bool is_local = false;
    int idx = -1;
protected:
    low_level_luau_context* ctx = nullptr;
    LuaObject(low_level_luau_context& ctx);
    LuaObject(low_level_luau_context& ctx, int local_idx);
    void push();
public:
    LuaObject toString();
    LuaString toStringRaw();
    LuaObject
};

class luau_context : private low_level_luau_context {

};

}//namespace godot

#endif