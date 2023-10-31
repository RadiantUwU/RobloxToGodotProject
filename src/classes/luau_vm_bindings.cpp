#include <classes/luau_vm.h>
#include <utils.h>

#include <cstdlib>
#include <godot_cpp/variant/utility_functions.hpp>
#include "luau_vm.h"


using namespace godot;


void LuauVM::_bind_passthrough_methods() {
    
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
        ClassDB::bind_method(D_METHOD("lua_loadstring", "code", "chunkname"), &LuauVM::load_string, DEFVAL("loadstring"));
        ClassDB::bind_method(D_METHOD("lua_dostring", "code", "chunkname"), &LuauVM::do_string, DEFVAL("dostring"));
        
        ClassDB::bind_method(D_METHOD("lua_tostring", "index"), &LuauVM::luaL_tostring);
    }

    // Auxiliary library
    {
        ClassDB::bind_method(D_METHOD("luaL_hasmetatable", "index", "tname"), &LuauVM::luaL_hasmetatable);

        ClassDB::bind_method(D_METHOD("luaL_error", "s"), &LuauVM::luaL_error);
        ClassDB::bind_method(D_METHOD("luaL_callmeta", "obj", "field"), &LuauVM::luaL_callmeta);
        ClassDB::bind_method(D_METHOD("luaL_getmetafield", "obj", "field"), &LuauVM::luaL_getmetafield);
        ClassDB::bind_method(D_METHOD("luaL_getmetatable", "tname"), &LuauVM::luaL_getmetatable);
        ClassDB::bind_method(D_METHOD("luaL_newmetatable", "tname"), &LuauVM::luaL_newmetatable);
        ClassDB::bind_method(D_METHOD("luaL_where", "lvl"), &LuauVM::luaL_where);
        ClassDB::bind_method(D_METHOD("luaL_typerror", "narg", "tname"), &LuauVM::luaL_typerror);
        ClassDB::bind_method(D_METHOD("luaL_argcheck", "cond", "narg", "msg"), &LuauVM::luaL_argcheck);

        ClassDB::bind_method(D_METHOD("luaL_checkany", "narg"), &LuauVM::luaL_checkany);
        ClassDB::bind_method(D_METHOD("luaL_checkint", "narg"), &LuauVM::luaL_checkint);
        ClassDB::bind_method(D_METHOD("luaL_checkstring", "narg"), &LuauVM::luaL_checkstring);
        ClassDB::bind_method(D_METHOD("luaL_checknumber", "narg"), &LuauVM::luaL_checknumber);
        ClassDB::bind_method(D_METHOD("luaL_checkvector", "narg"), &LuauVM::luaL_checkvector);
        ClassDB::bind_method(D_METHOD("luaL_checkobject", "narg"), &LuauVM::luaL_checkobject);
        ClassDB::bind_method(D_METHOD("luaL_checktype", "narg", "type"), &LuauVM::luaL_checktype);
        ClassDB::bind_method(D_METHOD("luaL_checkstack", "size", "message"), &LuauVM::luaL_checkstack);
        // ClassDB::bind_method(D_METHOD("luaL_checkudata", ""))
        ClassDB::bind_method(D_METHOD("luaL_checkoption", "narg", "array", "def"), &LuauVM::luaL_checkoption, DEFVAL(""));
    }
}


#pragma region RobloxToGodotProject


String (LuauVM::luaL_tostring)(int index) {
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


bool LuauVM::luaL_hasmetatable(int index, const String &tname) {
    return ::luaL_hasmetatable(L, index, tname.ascii().get_data());
}

void (LuauVM::luaL_error)(const String &err) {
    ::luaL_error(L, err.ascii().get_data());
}

bool LuauVM::luaL_callmeta(int obj, const String &field) {
    return ::luaL_callmeta(L, obj, field.ascii().get_data());
}

bool LuauVM::luaL_getmetafield(int obj, const String &field) {
    return ::luaL_getmetafield(L, obj, field.ascii().get_data());
}

bool (LuauVM::luaL_getmetatable)(const String &tname) {
    return ::lua_getfield(L, LUA_REGISTRYINDEX, tname.ascii().get_data());
}

bool LuauVM::luaL_newmetatable(const String &tname) {
    return ::luaL_newmetatable(L, tname.ascii().get_data());
}

void LuauVM::luaL_where(int lvl) {
    ::luaL_where(L, lvl);
}

void LuauVM::luaL_typerror(int nargs, const String &tname) {
    ::luaL_typeerror(L, nargs, tname.ascii().get_data());
}

void (LuauVM::luaL_argcheck)(bool cond, int narg, const String &msg) {
    ((void)((cond) ? (void)0 : ::luaL_argerror(L, narg, msg.ascii().get_data())));
}


void LuauVM::luaL_checkany(int narg) {
    ::luaL_checkany(L, narg);
}

int LuauVM::luaL_checkint(int narg) {
    return ::luaL_checkinteger(L, narg);
}

double LuauVM::luaL_checknumber(int narg) {
    return ::luaL_checknumber(L, narg);
}

String (LuauVM::luaL_checkstring)(int narg) {
    return ::luaL_checklstring(L, narg, NULL);
}


#if LUA_VECTOR_SIZE == 4
Vector4 LuauVM::luaL_checkvector(int narg) {
    const float *v = ::luaL_checkvector(L, narg);
    return Vector4(v[0], v[1], v[2], v[3]);
}
#else
Vector3 LuauVM::luaL_checkvector(int narg) {
    const float *v = ::luaL_checkvector(L, narg);
    return Vector3(v[0], v[1], v[2]);
}
#endif

Object *LuauVM::luaL_checkobject(int narg, bool valid) {
    return ::luaL_checkobject(L, narg, valid);
}

void LuauVM::luaL_checktype(int narg, int type) {
    ::luaL_checktype(L, narg, type);
}

void LuauVM::luaL_checkstack(int sz, const String &messsage) {
    ::luaL_checkstack(L, sz, messsage.ascii().get_data());
}

// luaL_checkudata

int LuauVM::luaL_checkoption(int narg, const Array &array, const String &def) {
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
