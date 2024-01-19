#include <classes/roblox_vm.h>
#include <utils.h>

#include <cstdlib>
#include <godot_cpp/variant/utility_functions.hpp>
#include "roblox_vm.h"


using namespace godot;


void RobloxVM::_bind_passthrough_methods() {
    
    // Constants
    {
        // Status
        BIND_ENUM_CONSTANT(LUA_OK);
        BIND_ENUM_CONSTANT(LUA_YIELD);
        BIND_ENUM_CONSTANT(LUA_BREAK);
        BIND_ENUM_CONSTANT(LUA_ERRERR);
        BIND_ENUM_CONSTANT(LUA_ERRMEM);
        BIND_ENUM_CONSTANT(LUA_ERRRUN);
        BIND_ENUM_CONSTANT(LUA_ERRSYNTAX);

        // Types
        BIND_ENUM_CONSTANT(LUA_TNIL);
        BIND_ENUM_CONSTANT(LUA_TBOOLEAN);
        BIND_ENUM_CONSTANT(LUA_TLIGHTUSERDATA);
        BIND_ENUM_CONSTANT(LUA_TNUMBER);
        BIND_ENUM_CONSTANT(LUA_TVECTOR);
        BIND_ENUM_CONSTANT(LUA_TSTRING);
        BIND_ENUM_CONSTANT(LUA_TTABLE);
        BIND_ENUM_CONSTANT(LUA_TFUNCTION);
        BIND_ENUM_CONSTANT(LUA_TUSERDATA);
        BIND_ENUM_CONSTANT(LUA_TTHREAD);
        // BIND_ENUM_CONSTANT(LUA_TPROTO);
        // BIND_ENUM_CONSTANT(LUA_TUPVAL);
        // BIND_ENUM_CONSTANT(LUA_TDEADKEY);
        // BIND_ENUM_CONSTANT(LUA_T_COUNT);

        // GC Operations
        BIND_ENUM_CONSTANT(LUA_GCSTOP);
        BIND_ENUM_CONSTANT(LUA_GCRESTART);
        BIND_ENUM_CONSTANT(LUA_GCCOLLECT);
        BIND_ENUM_CONSTANT(LUA_GCCOUNT);
        BIND_ENUM_CONSTANT(LUA_GCCOUNTB);
        BIND_ENUM_CONSTANT(LUA_GCISRUNNING);
        BIND_ENUM_CONSTANT(LUA_GCSTEP);
        BIND_ENUM_CONSTANT(LUA_GCSETGOAL);
        BIND_ENUM_CONSTANT(LUA_GCSETSTEPMUL);
        BIND_ENUM_CONSTANT(LUA_GCSETSTEPSIZE);

        // Other
        BIND_ENUM_CONSTANT(LUA_REGISTRYINDEX);
        BIND_ENUM_CONSTANT(LUA_GLOBALSINDEX);
        BIND_ENUM_CONSTANT(LUA_NOREF);
        BIND_ENUM_CONSTANT(LUA_REFNIL);
        BIND_ENUM_CONSTANT(LUA_MULTRET);
    }

    // Default library
    {
        ClassDB::bind_method(D_METHOD("lua_loadstring", "code", "chunkname"), &RobloxVM::load_string, DEFVAL("loadstring"));
        ClassDB::bind_method(D_METHOD("lua_dostring", "code", "chunkname"), &RobloxVM::do_string, DEFVAL("dostring"));
        
        ClassDB::bind_method(D_METHOD("lua_tostring", "index"), &RobloxVM::luaL_tostring);
    }

    // Auxiliary library
    {
        ClassDB::bind_method(D_METHOD("luaL_hasmetatable", "index", "tname"), &RobloxVM::luaL_hasmetatable);

        ClassDB::bind_method(D_METHOD("luaL_error", "s"), &RobloxVM::luaL_error);
        ClassDB::bind_method(D_METHOD("luaL_callmeta", "obj", "field"), &RobloxVM::luaL_callmeta);
        ClassDB::bind_method(D_METHOD("luaL_getmetafield", "obj", "field"), &RobloxVM::luaL_getmetafield);
        ClassDB::bind_method(D_METHOD("luaL_getmetatable", "tname"), &RobloxVM::luaL_getmetatable);
        ClassDB::bind_method(D_METHOD("luaL_newmetatable", "tname"), &RobloxVM::luaL_newmetatable);
        ClassDB::bind_method(D_METHOD("luaL_where", "lvl"), &RobloxVM::luaL_where);
        ClassDB::bind_method(D_METHOD("luaL_typerror", "narg", "tname"), &RobloxVM::luaL_typerror);
        ClassDB::bind_method(D_METHOD("luaL_argcheck", "cond", "narg", "msg"), &RobloxVM::luaL_argcheck);

        ClassDB::bind_method(D_METHOD("luaL_checkany", "narg"), &RobloxVM::luaL_checkany);
        ClassDB::bind_method(D_METHOD("luaL_checkint", "narg"), &RobloxVM::luaL_checkint);
        ClassDB::bind_method(D_METHOD("luaL_checkstring", "narg"), &RobloxVM::luaL_checkstring);
        ClassDB::bind_method(D_METHOD("luaL_checknumber", "narg"), &RobloxVM::luaL_checknumber);
        ClassDB::bind_method(D_METHOD("luaL_checkvector", "narg"), &RobloxVM::luaL_checkvector);
        ClassDB::bind_method(D_METHOD("luaL_checkobject", "narg"), &RobloxVM::luaL_checkobject);
        ClassDB::bind_method(D_METHOD("luaL_checktype", "narg", "type"), &RobloxVM::luaL_checktype);
        ClassDB::bind_method(D_METHOD("luaL_checkstack", "size", "message"), &RobloxVM::luaL_checkstack);
        // ClassDB::bind_method(D_METHOD("luaL_checkudata", ""))
        ClassDB::bind_method(D_METHOD("luaL_checkoption", "narg", "array", "def"), &RobloxVM::luaL_checkoption, DEFVAL(""));
    }
}


#pragma region RobloxToGodotProject


String (RobloxVM::luaL_tostring)(int index) {
	return String(::lua_tostring(L, index));
}

// lua_resume
// lua_touserdata
// lua_gethook
// lua_gethookcount
// lua_gethookmask
// lua_getinfo
// lua_getlocal
// lua_getstack
// lua_sethook
// lua_setlocal

#pragma endregion


#pragma region Auxiliary


bool RobloxVM::luaL_hasmetatable(int index, const String &tname) {
    return ::luaL_hasmetatable(L, index, tname.ascii().get_data());
}

void (RobloxVM::luaL_error)(const String &err) {
    ::luaL_error(L, err.ascii().get_data());
}

bool RobloxVM::luaL_callmeta(int obj, const String &field) {
    return ::luaL_callmeta(L, obj, field.ascii().get_data());
}

bool RobloxVM::luaL_getmetafield(int obj, const String &field) {
    return ::luaL_getmetafield(L, obj, field.ascii().get_data());
}

bool (RobloxVM::luaL_getmetatable)(const String &tname) {
    return ::lua_getfield(L, LUA_REGISTRYINDEX, tname.ascii().get_data());
}

bool RobloxVM::luaL_newmetatable(const String &tname) {
    return ::luaL_newmetatable(L, tname.ascii().get_data());
}

void RobloxVM::luaL_where(int lvl) {
    ::luaL_where(L, lvl);
}

void RobloxVM::luaL_typerror(int nargs, const String &tname) {
    ::luaL_typeerror(L, nargs, tname.ascii().get_data());
}

void (RobloxVM::luaL_argcheck)(bool cond, int narg, const String &msg) {
    ((void)((cond) ? (void)0 : ::luaL_argerror(L, narg, msg.ascii().get_data())));
}


void RobloxVM::luaL_checkany(int narg) {
    ::luaL_checkany(L, narg);
}

int RobloxVM::luaL_checkint(int narg) {
    return ::luaL_checkinteger(L, narg);
}

double RobloxVM::luaL_checknumber(int narg) {
    return ::luaL_checknumber(L, narg);
}

String (RobloxVM::luaL_checkstring)(int narg) {
    return ::luaL_checklstring(L, narg, NULL);
}


#if LUA_VECTOR_SIZE == 4
Vector4 RobloxVM::luaL_checkvector(int narg) {
    const float *v = ::luaL_checkvector(L, narg);
    return Vector4(v[0], v[1], v[2], v[3]);
}
#else
Vector3 RobloxVM::luaL_checkvector(int narg) {
    const float *v = ::luaL_checkvector(L, narg);
    return Vector3(v[0], v[1], v[2]);
}
#endif

Object *RobloxVM::luaL_checkobject(int narg, bool valid) {
    return ::luaL_checkobject(L, narg, valid);
}

void RobloxVM::luaL_checktype(int narg, int type) {
    ::luaL_checktype(L, narg, type);
}

void RobloxVM::luaL_checkstack(int sz, const String &messsage) {
    ::luaL_checkstack(L, sz, messsage.ascii().get_data());
}

// luaL_checkudata

int RobloxVM::luaL_checkoption(int narg, const Array &array, const String &def) {
    auto size = array.size();
    const char * * lst = (const char * *)memalloc(sizeof(const char *) * size);
    for (int64_t i = 0; i < size; i++) {
        Variant v = array[i];
        switch (v.get_type()) {
            case Variant::Type::STRING:
            case Variant::Type::STRING_NAME:
                lst[i] = (v.operator godot::String()).ascii().get_data();
            default:
                lst[i] = "";
        }
    }
    
    const char *def_p = NULL;
    if (!def.is_empty())
        def_p = def.ascii().get_data();
    int result = ::luaL_checkoption(L, narg, def_p, lst);
    memfree(lst);

    return result;
}


// luaL_optint
// luaL_optinteger
// luaL_optlong
// luaL_optlstring
// luaL_optnumber
// luaL_optstring


#pragma endregion
