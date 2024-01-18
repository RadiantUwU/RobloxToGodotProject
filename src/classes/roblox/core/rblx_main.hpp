#ifndef RBLX_MAIN
#define RBLX_MAIN

#include <cstddef>
#include <cstring>
#include <type_traits>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <lua.h>
#include <lualib.h>
#include <assert.h>
#include <mutex>
#include "rblx_debug.hpp"
#include "rblx_basic_types.hpp"

namespace godot {

struct RBXVariant;
class Instance;
class BaseScript;
class RobloxVMInstance;
class luau_State;
class luau_context;
class RBXScriptConnection;
class RBXScriptSignal;
class TaskScheduler;

class LuaObject final {
    luau_State *ls;
    int ref;
    friend class luau_State;
    friend class luau_context;
    LuaObject(luau_State *L);
    LuaObject(luau_State *L, int idx);
    void get(luau_State *to) const;
public:
    LuaObject(const LuaObject& o);
    ~LuaObject();
    bool operator==(const LuaObject& o) const;
    bool operator!=(const LuaObject& o) const;
};

class luau_State final {
    lua_State *L;
    RobloxVMInstance *vm;
    ::std::recursive_mutex mtx;
    Vector<int> tables_in_conversion;
    friend class LuaObject;
    friend class luau_context;
    lua_CFunction errhandle = nullptr;
public:
    luau_State(RobloxVMInstance *VM, lua_State *state);
    luau_State(RobloxVMInstance *vm);
    ~luau_State();

    lua_State *get_state() {
        return L;
    }
};

class luau_context { // middle level abstraction
private:
    static int thr_pcall(lua_State *L) {
        luau_context ctx = L;
        int r = ctx.pcall(ctx.get_stack_size()-2,LUA_MULTRET,1);
        if (r != ctx.get_stack_size()-1) {
            lua_error(L);
            return 0;
        }
        lua_remove(L, 1);
        return r;
    }
protected:
    lua_State *L;
    luau_State *ls;
    int64_t last_stack_size;
    int thr_ref;
    friend class LuaObject;
public:
    #pragma region USERDATA
    static constexpr int UD_TAXES = 1;
    static constexpr int UD_TBRICKCOLOR = 2;
    static constexpr int UD_TCATSEARCHPARAMS = 3;
    static constexpr int UD_TCFRAME = 4;
    static constexpr int UD_TCOLOR3 = 5;
    static constexpr int UD_TCOLORSEQ = 6;
    static constexpr int UD_TCOLORSEQKP = 7;
    static constexpr int UD_TDATE = 8;
    static constexpr int UD_TDOCKWIDGETPLUGINGUIINFO = 9;
    static constexpr int UD_TENUM = 10;
    static constexpr int UD_TENUMITEM = 11;
    static constexpr int UD_TENUMS = 12;
    static constexpr int UD_TFACES = 13;
    static constexpr int UD_TFLOATCURVEKP = 14;
    static constexpr int UD_TFONT = 15;
    static constexpr int UD_TINSTANCE = 16;
    static constexpr int UD_TNUMBERRANGE = 17;
    static constexpr int UD_TNUMBERSEQ = 18;
    static constexpr int UD_TNUMBERSEQKP = 19;
    static constexpr int UD_TOVERLAPPARAMS = 20;
    static constexpr int UD_TPATHWAYPOINT = 21;
    static constexpr int UD_TPHYSICALPROPERTIES = 22;
    static constexpr int UD_TRANDOM = 23;
    static constexpr int UD_TRAY = 24;
    static constexpr int UD_TRAYCASTPARAMS = 25;
    static constexpr int UD_TRAYCASTRESULT = 26;
    static constexpr int UD_TRBXSCRIPTCONNECTION = 27;
    static constexpr int UD_TRBXSCRIPTSIGNAL = 28;
    static constexpr int UD_TRECT = 29;
    static constexpr int UD_TREGION3 = 30;
    static constexpr int UD_TREGION3INT16 = 31;
    static constexpr int UD_TSHAREDTABLE = 32;
    static constexpr int UD_TTWEENINFO = 33;
    static constexpr int UD_TUDIM = 34;
    static constexpr int UD_TUDIM2 = 35;
    static constexpr int UD_TVECTOR2 = 36;
    static constexpr int UD_TVECTOR2INT16 = 37;
    static constexpr int UD_TVECTOR3 = 38;
    static constexpr int UD_TVECTOR3INT16 = 39;
    static constexpr const char * user_type_nil = "userdata";
    static constexpr const char * user_types[] = {
        nullptr, // reserved
        "Axes",
        "BrickColor",
        "CatalogSearchParams",
        "CFrame",
        "Color3",
        "ColorSequence",
        "ColorSequenceKeypoint",
        "DateTime",
        "DockWidgetPluginGuiInfo",
        "Enum",
        "EnumItem",
        "Enums",
        "Faces",
        "FloatCurveKey",
        "Font",
        "Instance",
        "NumberRange",
        "NumberSequence",
        "NumberSequenceKeypoint",
        "OverlapParams",
        "PathWaypoint",
        "PhysicalProperties",
        "Random",
        "Ray",
        "RaycastParams",
        "RaycastResult",
        "RBXScriptConnection",
        "RBXScriptSignal",
        "Rect",
        "Region3",
        "Region3int16",
        "SharedTable",
        "TweenInfo",
        "UDim",
        "UDim2",
        "Vector2",
        "Vector2int16",
        "Vector3",
        "Vector3int16"
    };
    #pragma endregion

    luau_context(lua_State *s) {
        L = s;
        getregistry("LUAU_STATE");
        ls = (luau_State*)lua_tolightuserdata(L, -1);
        lua_pop(L, 1);
        ls->mtx.lock();
        last_stack_size = lua_gettop(L);
        lua_pushthread(L);
        thr_ref = lua_ref(L, -1);
        lua_pop(L, 1);
    }
    luau_context(luau_State* state, lua_State *thr) {
        ls = state;
        ls->mtx.lock();
        L = thr;
        last_stack_size = lua_gettop(L);
        lua_pushthread(L);
        thr_ref = lua_ref(L, -1);
        lua_pop(L, 1);
    }
    luau_context(luau_State *state) {
        ls = state;
        ls->mtx.lock();
        L = state->L;
        last_stack_size = lua_gettop(L);
        lua_pushthread(L);
        thr_ref = lua_ref(L, -1);
        lua_pop(L, 1);
    }
    ~luau_context() {
        clear_stack();
        lua_unref(L, thr_ref);
        ls->mtx.unlock();
    }
    RBLX_INLINE void expect_empty_stack() {
        last_stack_size = 0;
    }
    RBLX_INLINE void dont_clear_stack() {
        last_stack_size = -1;
    }
    RBLX_INLINE void push_object() { ::lua_pushnil(L); }
    RBLX_INLINE void push_object(std::nullptr_t) { ::lua_pushnil(L); }
    RBLX_INLINE void push_object(bool b) { ::lua_pushboolean(L, b); }
    RBLX_INLINE void push_object(int64_t integer) { ::lua_pushinteger(L, integer); }
    RBLX_INLINE void push_object(int integer) { ::lua_pushinteger(L, integer); }
    RBLX_INLINE void push_object(double number) { ::lua_pushnumber(L, number); }
    RBLX_INLINE void push_object(lua_State *thr) { ::lua_pushthread(thr); ::lua_xmove(thr, L, 1); };
    RBLX_INLINE void push_object(const char* str) { ::lua_pushstring(L, str); }
    RBLX_INLINE void push_object(const char* str, size_t len) { ::lua_pushlstring(L, str, len); }
    RBLX_INLINE void push_object(lua_CFunction f, const char* fname = "<C++ context>") { lua_pushcfunction(L, f, fname); }
    RBLX_INLINE void push_object(void* p) { ::lua_pushlightuserdata(L, p); }
    RBLX_INLINE void push_cclosure(lua_CFunction f, int nup, const char* fname = "<C++ context>") { lua_pushcclosure(L, f, fname, nup); }
    RBLX_INLINE void push_object(const LuaObject& obj) { obj.get(ls); }
    RBLX_INLINE void push_object(const LuaString& s) { ::lua_pushlstring(L, s.s, s.l); }
    void push_object(const RBXVariant& v);
    void push_object(RBXScriptSignal* signal);
    void push_object(Instance* i);
    RBLX_INLINE void push_object(bool b, int idx) { ::lua_pushboolean(L, b); ::lua_insert(L, idx); }
    RBLX_INLINE void push_object(int64_t integer, int idx) { ::lua_pushinteger(L, integer); ::lua_insert(L, idx); }
    RBLX_INLINE void push_object(int integer, int idx) { ::lua_pushinteger(L, integer); ::lua_insert(L, idx); }
    RBLX_INLINE void push_object(double number, int idx) { ::lua_pushnumber(L, number); ::lua_insert(L, idx); }
    RBLX_INLINE void push_object(lua_State *thr, int idx) { ::lua_pushthread(thr); ::lua_xmove(thr, L, 1); ::lua_insert(L, idx); };
    RBLX_INLINE void push_object(const char* str, int idx) { ::lua_pushstring(L, str); ::lua_insert(L, idx); }
    RBLX_INLINE void push_object(const char* str, size_t len, int idx) { ::lua_pushlstring(L, str, len); ::lua_insert(L, idx); }
    RBLX_INLINE void push_object(const LuaObject& obj, int idx) { obj.get(ls); ::lua_insert(L, idx); }
    RBLX_INLINE void push_object(lua_CFunction f, const char* fname, int idx) { lua_pushcfunction(L, f, fname); ::lua_insert(L, idx); }
    RBLX_INLINE void push_object(void* p, int idx) { ::lua_pushlightuserdata(L, p); ::lua_insert(L, idx); }
    RBLX_INLINE void push_object(const LuaString& s, int idx) { ::lua_pushlstring(L, s.s, s.l); ::lua_insert(L, idx); }
    void push_object(const RBXVariant& v, int idx);
    void push_object(RBXScriptSignal* signal, int idx);
    void push_object(Instance* i, int idx);
    template <typename T, typename... Others>
    RBLX_INLINE int push_objects(T o, Others... others) { push_object(o); return push_objects(others...)+1; }
    template <typename T>
    RBLX_INLINE int push_objects(T o) { push_object(o); return 1; }
    RBLX_INLINE void push_current_thread() { ::lua_pushthread(L); }
    RBXVariant to_object();
    RBXVariant to_object(int idx);
    RBXVariant as_object();
    RBXVariant as_object(int idx);
    RBLX_INLINE lua_State* as_thread(int idx = -1) {return ::lua_tothread(L, idx); }
    RBLX_INLINE lua_CFunction as_cfunc(int idx = -1) {return ::lua_tocfunction(L, idx); }
    RBLX_INLINE int as_absolute_stack_index(int idx = -1) const {return (idx > 0) ? idx : ::lua_gettop(L)+1+idx;}
    RBLX_INLINE void push_pointer_hash(int idx) { ::lua_pushinteger(L, (size_t)::lua_topointer(L, idx)); }
    RBLX_INLINE int64_t as_pointer_hash(int idx) { return (size_t)::lua_topointer(L, idx); }

    RBLX_INLINE int get_type(int idx) { return ::lua_type(L, idx); }
    RBLX_INLINE int get_userdata_type(int idx) { return ::lua_userdatatag(L, idx); }
    RBLX_INLINE const char* get_typename(int idx) { return ::lua_typename(L, ::lua_type(L, idx)); }
    RBLX_INLINE const char* get_usertypename(int idx) {return (lua_type(L, idx) == LUA_TUSERDATA) ? user_types[::lua_userdatatag(L, idx)] : nullptr; }
    RBLX_INLINE const char* as_typename(int type) { return ::lua_typename(L, type); }
    RBLX_INLINE const char* as_usertypename(int utype) { return user_types[utype]; }
    RBLX_INLINE bool is_type(int idx, int type) { return ::lua_type(L, idx) == type; }
    RBLX_INLINE bool is_utype(int idx, int utype) { return (is_type(idx,LUA_TUSERDATA)) ? get_userdata_type(idx) == utype : false; }
    RBLX_INLINE bool is_cfunction(int idx) { return lua_iscfunction(L, idx); }

    RBLX_INLINE void insert_into(int idx) { return ::lua_insert(L, idx); }
    
    template <typename T, typename... Args>
    RBLX_INLINE T* new_userdata(int utype, Args... args) {
        T* p = new (::lua_newuserdatatagged(L, sizeof(T), utype)) T(args...);
        rawget(LUA_REGISTRYINDEX,"USERDATA_METATABLES");
        rawget(-1,utype);
        setmetatable(-3);
        pop_stack(1);
        rawget(LUA_REGISTRYINDEX,"USERDATA_REFS");
        push_value(-2);
        rawset(-2,(size_t)(void*)p);
        pop_stack(1);
        return p;
    }
    template <typename T>
    RBLX_INLINE T* new_instance(RobloxVMInstance* vm) {
        static_assert(::std::is_base_of<Instance, T>::value, "To pass a type to new_instance you must pass a class that derives from Instance!");
        T* p = new (::lua_newuserdatatagged(L, sizeof(T), UD_TINSTANCE)) T(vm);
        p->setName((const char*)p->ClassName);
        rawget(LUA_REGISTRYINDEX,"USERDATA_METATABLES");
        rawget(-1,UD_TINSTANCE);
        setmetatable(-3);
        pop_stack(1);
        rawget(LUA_REGISTRYINDEX,"USERDATA_REFS");
        push_value(-2);
        rawset(-2,(size_t)(void*)p);
        pop_stack(1);
        return p;
    }
    template <typename T>
    RBLX_INLINE T* as_userdata(int idx) { //unsafe
        return (T*)::lua_touserdata(L, idx);
    }
    template <typename T>
    RBLX_INLINE T* as_userdata(int idx, int utype) { //safe
        return (T*)::lua_touserdatatagged(L, idx, utype);
    }
    template <typename T>
    RBLX_INLINE T& as_userdata_ref(int idx) {
        return *(T*)::lua_touserdata(L, idx);
    }
    template <typename T>
    RBLX_INLINE T& as_userdata_ref(int idx, int utype) {
        return *(T*)::lua_touserdatatagged(L, idx, utype);
    }
    RBLX_INLINE void set_dtor(int tag, lua_Destructor dtor) { ::lua_setuserdatadtor(L, tag, dtor); }
    RBLX_INLINE void setmetatable(int object_idx) {
        ::lua_setmetatable(L, object_idx);
    }
    RBLX_INLINE void setmetatable(int object_idx, int metatable_idx) {
        ::lua_pushvalue(L, metatable_idx);
        ::lua_setmetatable(L, (object_idx < 0) ? object_idx-1 : object_idx);
        ::lua_remove(L, metatable_idx);
    }
    RBLX_INLINE int getmetatable(int object_idx) {
        return ::lua_getmetatable(L, object_idx);
    }
    RBLX_INLINE void newmetatable_type(int utype) {
        rawget(LUA_REGISTRYINDEX,"USERDATA_METATABLES");
        new_table();
        push_value(-1);
        rawset(-3,utype);
    }

    RBLX_INLINE void freeze(int tbl_idx) {
        ::lua_setreadonly(L, tbl_idx, true);
    }
    RBLX_INLINE void unfreeze(int tbl_idx) {
        ::lua_setreadonly(L, tbl_idx, false);
    }
    RBLX_INLINE bool is_frozen(int tbl_idx) {
        return ::lua_getreadonly(L, tbl_idx);   
    }

    RBLX_INLINE bool has_argument(int argn) { return !lua_isnone(L, argn); }
    RBLX_INLINE void assert_has_argument(int argn, const char* argname) {
        if (lua_isnone(L, argn)) 
            errorf("missing argument #%d to '%s' (any expected)", argn, argname);
    }
    RBLX_INLINE void assert_type_argument(int argn, const char* argname, int type) {
        if (lua_isnone(L, argn)) 
            errorf("missing argument #%d to '%s' (%s expected)", argn, argname, lua_typename(L, type));
        if (!is_type(argn, type)) 
            errorf("invalid argument #%d to '%s' (%s expected, got %s)", argn, argname, lua_typename(L, type), get_typename(argn));
    }
    RBLX_INLINE void assert_usertype_argument(int argn, const char* argname, int utype) {
        if (lua_isnone(L, argn)) 
            errorf("missing argument #%d to '%s' (%s expected)", argn, argname, user_types[utype]);
        if (!lua_isuserdata(L, argn)) 
            errorf("invalid argument #%d to '%s' (%s expected, got %s)", argn, argname, user_types[utype], get_typename(argn));
        if (lua_userdatatag(L, argn) != utype) 
            errorf("invalid argument #%d to '%s' (%s expected, got %s)", argn, argname, user_types[utype], user_types[lua_userdatatag(L, argn)]);
    }
    RBLX_INLINE void assert_type_argument(int argn, const char* argname, int type1, int type2) {
        if (lua_isnone(L, argn)) 
            errorf("missing argument #%d to '%s' (%s|%s expected)", argn, argname, lua_typename(L, type1), lua_typename(L, type2));
        if (!is_type(argn, type1) and !is_type(argn, type2)) 
            errorf("invalid argument #%d to '%s' (%s|%s expected, got %s)", argn, argname, lua_typename(L, type1), lua_typename(L, type2), get_typename(argn));
    }
    RBLX_INLINE void assert_stack_size(int x) {
        if (lua_gettop(L) != x)
            errorf("expected %d arguments, got %d",x,lua_gettop(L));
    }
    RBLX_INLINE void assert_stack_size(int req, int opt) {
        if (lua_gettop(L) > req+opt)
            errorf("expected %d-%d arguments, got %d",req,req+opt,lua_gettop(L));
        if (lua_gettop(L) < req)
            errorf("expected %d-%d arguments, got %d",req,req+opt,lua_gettop(L));
    }
    RBLX_INLINE void assert_stack_size_vararg(int req) {
        if (lua_gettop(L) < req)
            errorf("expected %d arguments minimum, got %d",req,lua_gettop(L));
    }
    RBLX_INLINE void assert_stack_size_vararg(int req, int opt) {
        if (lua_gettop(L) < req)
            errorf("expected %d arguments minimum, got %d",req,lua_gettop(L));
    }

    RBLX_INLINE void pop_stack(int n) { lua_pop(L, n); }
    RBLX_INLINE void remove_stack(int idx) { lua_remove(L, idx); }
    RBLX_INLINE int get_stack_size() const { return lua_gettop(L); }
    RBLX_INLINE void push_value(int idx) { lua_pushvalue(L, idx); }
    RBLX_INLINE void push_value(int idx, int idxdest) { lua_pushvalue(L, idx); lua_insert(L, idxdest); }
    RBLX_INLINE void clear_stack() { 
        if (last_stack_size < 0) return; 
        int64_t s = lua_gettop(L) - last_stack_size;
        if (s>0) {
            lua_pop(L, s);
            RBLX_PRINT_VERBOSE("popped ",s," values from clear_stack()");
        }
    }

    RBLX_INLINE int get(int tbl_idx) { return lua_gettable(L, tbl_idx); }
    RBLX_INLINE int get(int tbl_idx, const char* field) { return lua_getfield(L, tbl_idx, field); }
    RBLX_INLINE int get(int tbl_idx, int64_t i) { lua_pushnumber(L, i); return lua_gettable(L, (tbl_idx < 0 && !lua_ispseudo(tbl_idx)) ? tbl_idx-1 : tbl_idx); }
    RBLX_INLINE void set(int tbl_idx) { lua_settable(L, tbl_idx); }
    RBLX_INLINE void set(int tbl_idx, const char* field) { lua_setfield(L, tbl_idx, field); }
    RBLX_INLINE void set(int tbl_idx, int64_t i) { lua_pushnumber(L, i); lua_pushvalue(L, -2); lua_settable(L, (tbl_idx < 0 && !lua_ispseudo(tbl_idx)) ? tbl_idx-2 : tbl_idx); }
    RBLX_INLINE int rawget(int tbl_idx) { return lua_rawget(L, tbl_idx); }
    RBLX_INLINE int rawget(int tbl_idx, const char* field) { return lua_rawgetfield(L, tbl_idx, field); }
    RBLX_INLINE int rawget(int tbl_idx, int64_t i) { return lua_rawgeti(L, tbl_idx, i); }
    RBLX_INLINE void rawset(int tbl_idx) { lua_rawset(L, tbl_idx); }
    RBLX_INLINE void rawset(int tbl_idx, const char* field) { lua_rawsetfield(L, tbl_idx, field); }
    RBLX_INLINE void rawset(int tbl_idx, int64_t i) { lua_rawseti(L, tbl_idx, i); }
    RBLX_INLINE int len(int idx) { return lua_objlen(L, idx); } // This operation does *NOT* pop from stack!

    RBLX_INLINE int rawiter(int idx, int iter) { return lua_rawiter(L, idx, iter); }
    RBLX_INLINE int next(int idx) { return lua_next(L, idx); }

    RBLX_INLINE void new_table() { lua_newtable(L); }
    RBLX_INLINE void new_array(int n) { lua_createtable(L, n, 0); }
    RBLX_INLINE void new_dictionary(int n) { lua_createtable(L, 0, n); }
    RBLX_INLINE void create_array_from_stack(int nargs) { // pops arguments and replaces them with a table
        new_array(nargs);
        lua_insert(L, -nargs-1);
        int tbl_idx = as_absolute_stack_index(-nargs-1);
        for (int i = nargs; i > 0; i--) {
            lua_rawseti(L, tbl_idx, i);
        }
    }
    // pops array and pushes all arguments, returns how many arguments were pushed
    RBLX_INLINE int64_t push_vararg_array_and_pop(int tbl_idx = -1) { 
        tbl_idx = as_absolute_stack_index(tbl_idx);
        int64_t nargs = len(tbl_idx);
        for (int64_t iter = 1; iter <= nargs; iter++) {
            rawget(tbl_idx,iter);
        }
        lua_remove(L, tbl_idx);
        return nargs;
    }
    RBLX_INLINE void clone_table(int tbl_idx = -1) {
        ::lua_pushvalue(L, tbl_idx);
        ::lua_newtable(L);
        int iter = 0;
        while (true) {
            iter = rawiter(-2, iter);
            if (iter == -1) {
                ::lua_remove(L, -2);
                return;
            }
            ::lua_rawset(L, -3);
        }
    }
    RBLX_INLINE void clear_table(int tbl_idx = -1) { ::lua_cleartable(L, tbl_idx); }

    RBLX_INLINE int getglobal(const char* name) { return lua_rawgetfield(L, LUA_GLOBALSINDEX, name); }
    RBLX_INLINE void setglobal(const char* name) { lua_rawsetfield(L, LUA_GLOBALSINDEX, name); }
    RBLX_INLINE int getregistry(const char* name) { return lua_rawgetfield(L, LUA_REGISTRYINDEX, name); }
    RBLX_INLINE void setregistry(const char* name) { lua_rawsetfield(L, LUA_REGISTRYINDEX, name); }

    RBLX_INLINE void call(int nargs, int nres) { lua_call(L, nargs, nres); }
    RBLX_INLINE int pcall(int nargs, int nres, int errfuncidx = 0) {
        return lua_pcall(L, nargs, nres, errfuncidx);
    }
    RBLX_INLINE double clock() {
        return lua_clock();
    }
    [[noreturn]] RBLX_INLINE void error() {
        lua_error(L);
    }
    [[noreturn]] RBLX_INLINE void error(const char *s) {
        lua_pushstring(L, s);
        lua_error(L);
    }
    template <typename T>
    [[noreturn]] RBLX_INLINE void errorf(const char *fmt, T args) {
        lua_pushfstringL(L, fmt, args);
        lua_error(L);
    }
    template <typename... T>
    [[noreturn]] RBLX_INLINE void errorf(const char *fmt, T... args) {
        lua_pushfstringL(L, fmt, args...);
        lua_error(L);
    }

    // NOTE: This function returns -1 so that Luau actually knows it yielded!
    RBLX_INLINE int yield(int nargs) {
        return lua_yield(L, nargs);
    }
    // MAIN: [...], [coro], [args] -> [...], [res]
    // CORO: [...] -> [...]
    int resume(int nargs, int nres) {
        int stack_size = lua_gettop(L)-nargs;
        int retnres;
        lua_State* thr = lua_tothread(L, -1-nargs);
        if (thr == nullptr) errorf("Internal error: expected thread at position %d(%d), got a non-thread object/null.",-1-nargs,as_absolute_stack_index(-1-nargs));
        lua_xmove(L, thr, nargs);
#ifndef NDEBUG
        {
            luau_context ctx = thr;
            RBLX_PRINT_VERBOSE("Resuming thread! Current stack: ");
            ctx.print_stack_absolute();
        }
#endif
        int status = lua_resume(thr, L, nargs);
        retnres = lua_gettop(thr);
        lua_pop(L, 1); // pop the thread off
        if (status == LUA_OK or status == LUA_YIELD) {
            lua_xmove(thr, L, retnres);
            if (nres > retnres) {
                for (int i = 0; i < nres-retnres; i++) {
                    lua_pushnil(L);
                }
            } else if (nres < retnres) {
                lua_pop(L, retnres-nres);
            }
        }
        return status;
    }
    // [...], [Function], [nargs] -> [...], [thread]
    RBLX_INLINE lua_State* new_thread(int nargs) { 
        lua_State* thr = lua_newthread(L);
        insert_into(-2-nargs);
        lua_xmove(L, thr, 1+nargs);
        return thr; // if they wanna use it anyways, its on stack
    }
    // [...], [Function], [nargs] -> [...], [thread]
    RBLX_INLINE lua_State* new_thread(int nargs, BaseScript* attached_script) { 
        lua_State* thr = lua_newthread(L);
        insert_into(-2-nargs);
        lua_xmove(L, thr, 1+nargs);
        lua_setthreaddata(thr, attached_script);
        return thr; // if they wanna use it anyways, its on stack
    }
    RBLX_INLINE BaseScript* get_attached_script() {
        return (BaseScript*)lua_getthreaddata(L);
    }
    RBLX_INLINE BaseScript* get_attached_script(lua_State *thr) {
        return (BaseScript*)lua_getthreaddata(thr);
    }
    // [...], [thread] -> [...]
    RBLX_INLINE int status() {
        lua_State* thr = lua_tothread(L, -1);
        int status = lua_status(thr);
        lua_pop(L, 1);
        return status;
    }
    // [...] -> [...]
    RBLX_INLINE int status(lua_State *thr) {
        return lua_status(thr);
    }
    // [...], [thread] -> [...]
    RBLX_INLINE void close() {
        lua_State* thr = lua_tothread(L, -1);
        if (thr != L and lua_mainthread(L) != thr) lua_resetthread(thr);
        lua_pop(L, 1);
    }
    // [...] -> [...]
    RBLX_INLINE void close(lua_State *thr) {
        if (thr != L and lua_mainthread(L) != thr) lua_resetthread(thr);
    }
    // [...], [thread] -> [...]
    RBLX_INLINE int costatus() {
        lua_State* thr = lua_tothread(L, -1);
        int status = lua_costatus(L, thr);
        lua_pop(L, 1);
        return status;
    }
    // [...] -> [...]
    RBLX_INLINE int costatus(lua_State *thr) {
        return lua_costatus(L, thr);
    }
    RBLX_INLINE bool can_yield() {
        return lua_isyieldable(L);
    }

    lua_Debug* getinfo(const char* what) {
        lua_Debug* d = new lua_Debug;
        if (!lua_getinfo(L, 1, what, d)) {
            delete d;
            return nullptr;
        }
        return d;
    }
    lua_Debug* getinfo(const char* what, int level) {
        lua_Debug* d = new lua_Debug;
        if (!lua_getinfo(L, level, what, d)) {
            delete d;
            return nullptr;
        }
        return d;
    }
    RBLX_INLINE void dbg_break() { ::lua_break(L); }

    RBLX_INLINE const char* getlocal(int level, int n) {
        return lua_getlocal(L, level, n);
    }
    RBLX_INLINE void getfenv(int funcidx) { lua_getfenv(L, funcidx); }
    RBLX_INLINE void setfenv(int funcidx) { lua_setfenv(L, funcidx); }
    RBLX_INLINE const char* getupvalue(int funcidx, int n) { return lua_getupvalue(L, funcidx, n); }
    RBLX_INLINE const char* getupvalue_level(int level, int n) { 
        delete getinfo("f",level);
        const char *ret = lua_getupvalue(L, -1, n);
        lua_remove(L, (ret == nullptr) ? -1 : -2);
        return ret;
    }
    RBLX_INLINE const char* setupvalue(int funcidx, int n) { return lua_setupvalue(L, funcidx, n); }
    RBLX_INLINE const char* setupvalue_level(int level, int n) { 
        delete getinfo("f",level);
        const char *ret = lua_setupvalue(L, -1, n);
        lua_remove(L, (ret == nullptr) ? -1 : -2);
        return ret;
    }
    RBLX_INLINE void getfenv_level(int level) { 
        delete getinfo("f",level);
        lua_getfenv(L, -1);
        lua_remove(L, -2);
    }
    RBLX_INLINE void setfenv_level(int level) { 
        delete getinfo("f",level);
        lua_insert(L, -2);
        lua_setfenv(L, -2); 
        lua_remove(L, -1);
    }

    RBLX_INLINE int64_t new_ref(int idx) { int ref = lua_ref(L, idx); lua_remove(L, idx); return ref; }
    RBLX_INLINE void push_ref(int64_t ref) { lua_getref(L, ref); }
    RBLX_INLINE void delete_ref(int64_t ref) { lua_unref(L, ref); }

    RBLX_INLINE void move_args(lua_State *to, int amount) {::lua_xmove(L, to, amount);}
    RBLX_INLINE void copy_arg(lua_State *to, int idx) {::lua_xpush(L, to, idx); }

    RBLX_INLINE bool rawequal(int idx1, int idx2) { return ::lua_rawequal(L, idx1, idx2); }
#ifndef NDEBUG
#include <iostream>
private:
    RBLX_NOINLINE void _print_stack(int n = -1) {
        if (n == -1) {
            bool began = true;
            std::cout << "[ ";
            for (int i = 1; i <= get_stack_size(); i++) {
                if (!began) std::cout<<", ";
                if (is_type(i, LUA_TTABLE)) {
                    std::cout << "{ len: " << len(i) << " }";
                } else {
                    std::cout << ((is_type(i, LUA_TUSERDATA)) ? get_usertypename(i) : get_typename(i));
                }
                began = false;
            }
        } else {
            bool began = true;
            std::cout << "[ ..., ";
            int stk_size = get_stack_size();
            for (int i = n; i <= stk_size; i++) {
                if (!began) std::cout<<", ";
                if (is_type(i, LUA_TTABLE)) {
                    std::cout << "{ len: " << len(i) << " }";
                } else {
                    std::cout << ((is_type(i, LUA_TUSERDATA)) ? get_usertypename(i) : get_typename(i));
                }
                began = false;
            }
        }
        std::cout << " ]\n";
    }
public:
    RBLX_NOINLINE void print_stack() {
        if (last_stack_size == -1) print_stack_absolute();
        else _print_stack(last_stack_size);
    }
    RBLX_NOINLINE void print_stack_absolute() {
        _print_stack();
    }
#endif

    RBLX_INLINE RobloxVMInstance* get_vm() {
        getregistry("ROBLOX_VM");
        RobloxVMInstance* vm = (RobloxVMInstance*)::lua_tolightuserdata(L, -1);
        lua_pop(L, 1);
        return vm;
    }
    TaskScheduler* get_task_scheduler();

    // Returns a LUA_STATUS
    int compile(const char* fname, LuaString code, int env_idx = 0);

    RBLX_INLINE luau_State* get_luau_state() { return ls; }

    RBLX_INLINE bool is_running() {auto s = lua_costatus(L, as_thread(-1)); pop_stack(-1); return s==LUA_COSUS || s==LUA_CORUN;}
    RBLX_INLINE bool is_running(lua_State* thr) {auto s = lua_costatus(L, thr); return s==LUA_COSUS || s==LUA_CORUN;}

    RBLX_INLINE bool ready_to_resume(lua_State* thr) {
        auto costatus = lua_costatus(L, thr);
        if (costatus==LUA_COSUS && lua_status(thr) == LUA_OK) { // suspended but not yielded
            //check if a function is first argument
            return lua_type(thr,1)==LUA_TFUNCTION;
        } else return costatus==LUA_COSUS;
    }
    RBLX_INLINE bool ready_to_resume() {
        lua_State *thr = as_thread(-1); pop_stack(-1);
        return ready_to_resume(thr);
    }
};

class luau_function_context : public luau_context {
public:
    luau_function_context(lua_State *L) : luau_context(L) {
        dont_clear_stack();
    }
    luau_function_context(luau_State *L) : luau_context(L) {
        dont_clear_stack();
    }
    luau_function_context(luau_State *L, lua_State *thr) : luau_context(L,thr) {
        dont_clear_stack();
    }
    int lua_return(int nargs) const {
#ifndef NDEBUG
        assert(nargs <= get_stack_size());
#endif
        return nargs;
    }
};

struct RBXVariant final {
private:
    union {
        int64_t integer = 0;
        bool boolean;
        double number;
        LuaObject obj;
        struct { // string
            char* str;
            size_t strl;
        };
        void* ptr;
    };
public:
    enum class Type {
        RBXVARIANT_NIL,
        RBXVARIANT_BOOL,
        RBXVARIANT_INT,
        RBXVARIANT_NUM,
        RBXVARIANT_OBJ,
        RBXVARIANT_STR,
        RBXVARIANT_PTR
    } type = Type::RBXVARIANT_NIL;
    RBXVariant() : type(Type::RBXVARIANT_NIL) {}
    RBXVariant(bool b) : type(Type::RBXVARIANT_BOOL), boolean(b) {}
    RBXVariant(int64_t i) : type(Type::RBXVARIANT_INT), integer(i) {}
    RBXVariant(double n) : type(Type::RBXVARIANT_NUM), number(n) {}
    RBXVariant(LuaObject& o) : type(Type::RBXVARIANT_OBJ), obj(o) { RBLX_PRINT_VERBOSE("Initializing RBXVariant with obj ", &o);}
    RBXVariant(const char *s) : type(Type::RBXVARIANT_STR) {
        if (s == nullptr) {
            str = (char*)memalloc(sizeof(char));
            str[0] = '\0';
        } else {
            strl = strlen(s)+1;
            str = (char*)memalloc((strl+1)*sizeof(char));
            strcpy(str, s);
            RBLX_PRINT_VERBOSE("Initializing RBXVariant with str ", str, " of len ", strl);
        }
    }
    RBXVariant(const char* s, size_t l) : type(Type::RBXVARIANT_STR) {
        strl = l;
        if (s == nullptr) {
            str = (char*)memalloc(sizeof(char));
            str[0] = '\0';
        } else {
            str = (char*)memalloc((strl+1)*sizeof(char));
            memcpy(str, s, (strl+1)*sizeof(char));
        }
    }
    RBXVariant(void* p) : type(Type::RBXVARIANT_PTR), ptr(p) {}
    RBXVariant(const RBXVariant& other) : type(other.type) {
        switch (type) {
        case Type::RBXVARIANT_BOOL:
            boolean = other.boolean;
            break;
        case Type::RBXVARIANT_INT:
            integer = other.integer;
            break;
        case Type::RBXVARIANT_NUM:
            number = other.number;
            break;
        case Type::RBXVARIANT_OBJ:
            obj = other.obj;
            break;
        case Type::RBXVARIANT_STR:
            strl = other.strl;
            str = new char[strl+1];
            memcpy(str,other.str,(strl+1)*sizeof(char));
            break;
        case Type::RBXVARIANT_PTR:
            ptr = other.ptr;
        default:
            break;
        }
    }
    RBXVariant& operator=(const RBXVariant& other) {
        type = other.type;
        switch (type) {
        case Type::RBXVARIANT_BOOL:
            boolean = other.boolean;
            break;
        case Type::RBXVARIANT_INT:
            integer = other.integer;
            break;
        case Type::RBXVARIANT_NUM:
            number = other.number;
            break;
        case Type::RBXVARIANT_OBJ:
            obj = other.obj;
            break;
        case Type::RBXVARIANT_STR:
            strl = other.strl;
            str = (char*)memalloc((strl+1)*sizeof(char));
            memcpy(str,other.str,(strl+1)*sizeof(char));
            break;
        case Type::RBXVARIANT_PTR:
            ptr = other.ptr;
        default:
            break;
        }
        return *this;
    }
    ~RBXVariant() {
        switch (type) {
        case Type::RBXVARIANT_OBJ:
            obj.~LuaObject();
            break;
        case Type::RBXVARIANT_STR:
            memfree(str);
            break;
        default:
            break;
        }
    }
    operator bool() const {
        switch (type) {
        case Type::RBXVARIANT_NIL: return false;
        case Type::RBXVARIANT_NUM: return number > 0;
        case Type::RBXVARIANT_BOOL: return boolean;
        case Type::RBXVARIANT_INT: return integer != 0;
        default: return true;
        }
    }
    operator int64_t() const {
        switch (type) {
        case Type::RBXVARIANT_NIL: return 0;
        case Type::RBXVARIANT_NUM: return (int)number;
        case Type::RBXVARIANT_BOOL: return boolean;
        case Type::RBXVARIANT_INT: return integer;
        case Type::RBXVARIANT_PTR: return (size_t)ptr;
        default: return 1;
        }
    }
    operator double() const {
        switch (type) {
        case Type::RBXVARIANT_NIL: return 0.0;
        case Type::RBXVARIANT_NUM: return number;
        case Type::RBXVARIANT_BOOL: return (double)boolean;
        case Type::RBXVARIANT_INT: return integer;
        default: return NAN;
        }
    }
    operator LuaObject() const {
        return LuaObject(obj);
    }
    operator LuaObject&() {
        return obj;
    }
    typedef const LuaObject ConstLuaObject;
    operator ConstLuaObject&() const {
        return obj;
    }
    int get_slen() const {
        return strl;
    }
    const char *get_str() const {
        return str;
    }
    operator void*() const {
        return ptr;
    }
    operator LuaString() {
        return LuaString(get_str(),get_slen());
    }
    bool operator==(const RBXVariant& other) const {
        if ((other.type != type) and not ((other.type == Type::RBXVARIANT_INT or other.type == Type::RBXVARIANT_NUM) and (type == Type::RBXVARIANT_INT or type == Type::RBXVARIANT_NUM))) return false;
        switch (type) {
        case Type::RBXVARIANT_NIL:
            return 1;
        case Type::RBXVARIANT_BOOL:
            return boolean==other.boolean;
        case Type::RBXVARIANT_INT:
        case Type::RBXVARIANT_NUM:
            return integer==other.integer or number == other.number or integer == other.number or number == other.integer;
        case Type::RBXVARIANT_STR:
            if (get_slen() != other.get_slen()) return false;
            return memcmp(get_str(),other.get_str(),sizeof(char)*get_slen()) == 0;
        case Type::RBXVARIANT_PTR:
            return ptr == other.ptr;
        case Type::RBXVARIANT_OBJ:
            return obj == other.obj;
        default:
            return false;
        }
    }
    bool operator!=(const RBXVariant& other) const {
        return not (*this == other);
    };
};

class TaskScheduler {
    RobloxVMInstance* vm;
    friend class RobloxVMInstance;
    TaskScheduler(RobloxVMInstance *vm);
protected:
    void dispatch_thread(luau_context& ctx, size_t nargs);
public: 
    ~TaskScheduler();
    static int lua_task_error_handler(lua_State *L);
    static int lua_task_spawn(lua_State *L);
    static int lua_task_defer(lua_State *L);
    static int lua_task_delay(lua_State *L);
    static int lua_task_desynchronize(lua_State *L);// TODO: To be implemented...
    static int lua_task_synchronize(lua_State *L);// TODO: To be implemented...
    static int lua_task_wait(lua_State *L);
    static int lua_task_cancel(lua_State *L);

    bool dispatch(lua_State *L);
    bool dispatch(luau_State *L);

    bool dead(luau_State *L);
};
enum RBLX_VMRunContext {
    RUNCTXT_CORE,
    RUNCTXT_LOCAL,
    RUNCTXT_PLUGIN,
    RUNCTXT_NORMAL
};
class RobloxVMInstance final {
    ::std::recursive_mutex mtx;

    friend class luau_State;

    // Source: https://github.com/Fumohouse/godot-luau-script
    // Based on the default implementation seen in the Lua 5.1 reference
    static void *lua_alloc(void *, void *ptr, size_t, size_t nsize) {
        if (nsize == 0) {
            // Lua assumes free(NULL) is ok. For Godot it is not.
            if (ptr)
                memfree(ptr);

            return nullptr;
        }

        return memrealloc(ptr, nsize);
    }
    void register_types(lua_State *L);
    void register_genv(lua_State *L);
    void register_registry(lua_State *L);
    lua_State* create_lua_state() {
        lua_State *L = lua_newstate(lua_alloc, nullptr);
        ::lua_pushlightuserdata(L, this);
        ::lua_rawsetfield(L, LUA_REGISTRYINDEX, "ROBLOX_VM");
        register_types(L);
        return L;
    }
    static int lua_typeof(lua_State *L) {
        luau_function_context lc = L;
        lc.assert_stack_size(0,1);
        if (lc.get_stack_size() == 0) {
            lc.push_object("nil");
            return 1;
        }
        switch (lc.get_type(1)) {
            case LUA_TNIL:
            case LUA_TBOOLEAN:
            case LUA_TNUMBER:
            case LUA_TSTRING:
            case LUA_TTABLE:
            case LUA_TFUNCTION:
            case LUA_TTHREAD:
            case LUA_TLIGHTUSERDATA: // you anyways shouldnt have ur hand on this type of object
                lc.push_object(lc.get_typename(1));
                break;
            case LUA_TUSERDATA: {
                int utype = lc.get_userdata_type(1);
                if (utype == -1 or utype == 0) {
                    lc.push_object("userdata");
                } else {
                    lc.push_object(lc.get_usertypename(1));
                }
                break;
            }
            default:
                RBLX_PRINT_VERBOSE("unknown type??");
                lc.error("Internal error: unknown type given to typeof");
        }
        return 1;
    }
    static int lua_getmetatable_override(lua_State *L) {
        luau_context ls = L;
        ls.assert_stack_size(1);
        ls.assert_type_argument(1,"t",LUA_TTABLE,LUA_TUSERDATA);
        if (ls.is_type(1, LUA_TTABLE) or ls.is_utype(1, -1)) {
            ls.rawget(LUA_REGISTRYINDEX,"default_getmetatable");
            ls.insert_into(1);
            ls.call(1, 1);
            return 1;
        }
        return 0;
    }
    static int lua_setmetatable_override(lua_State *L) {
        luau_context ls = L;
        ls.assert_stack_size(1,2);
        ls.assert_type_argument(1,"t",LUA_TTABLE,LUA_TUSERDATA);
        if (!ls.has_argument(2)) 
            ls.push_object();
        else 
            ls.assert_type_argument(2,"mt",LUA_TTABLE,LUA_TNIL);

        if (ls.is_type(1, LUA_TTABLE) or ls.is_utype(1, -1)) {
            ls.rawget(LUA_REGISTRYINDEX,"default_setmetatable");
            ls.insert_into(1);
            ls.call(2, 0);
        }
        return 0;
    }

public:
    RobloxVMInstance(lua_State* main);
    ~RobloxVMInstance();

    luau_State* main_synchronized;
    Instance* game;
    
    //SERVICES


    //misc
    TaskScheduler* task;
    RBLX_VMRunContext context = RUNCTXT_NORMAL;

    LuaString open_script_asset(LuaString asset) {
        return nullptr; // TODO: implement this
    };
};

}

#endif