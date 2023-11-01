#include "rblx_instance.hpp"
#include "rblx_main.hpp"
#include "rblx_basic_types.hpp"
#include "rblx_debug.hpp"
#include <lua.h>

namespace godot {

int Instance::new_instance(lua_State *L) {
    luau_function_context fn = L;
    fn.assert_type_argument(1,"ClassName",LUA_TSTRING);
    RBXVariant v_temp = fn.as_object(1);
    LuaString s = LuaString(v_temp.get_str(),v_temp.get_slen());
    RobloxVMInstance *vm;
    fn.rawget(LUA_REGISTRYINDEX,"ROBLOX_VM");
    v_temp = fn.to_object();
    vm = (RobloxVMInstance*)(void*)v_temp;
    RBLX_PRINT_VERBOSE((void*)vm);
    if (s == "Instance") {// TODO: illegal on roblox lmaooo
        fn.new_instance<Instance>(vm);
        return 1;
    }
    fn.errorf("Invalid class name provided to ClassName '%s'",s.s);
    return 0;
}

}