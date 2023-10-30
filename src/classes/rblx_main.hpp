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
#include <mutex>

namespace godot {

struct RBXVariant;
class Instance;
class RobloxVMInstance;
class luau_State;

class luau_context;

class LuaObject final {
    luau_State *ls;
    int ref;
    friend class luau_State;
    friend class luau_context;
    LuaObject(luau_State *L);
    LuaObject(luau_State *L, int idx);
    void get(luau_State *to);
public:
    LuaObject(LuaObject& o);
    ~LuaObject();
};

class luau_State final {
    lua_State *L;
    RobloxVMInstance *vm;
    ::std::recursive_mutex mtx;
    Vector<int> tables_in_conversion;
    friend class LuaObject;
    friend class luau_context;
public:
    luau_State(RobloxVMInstance *VM, lua_State *state) {
        L = state;
        ::lua_pushlightuserdata(L, this);
        ::lua_rawsetfield(L, LUA_REGISTRYINDEX, "LUAU_STATE");
        ::lua_newtable(L);
        ::lua_rawsetfield(L, LUA_REGISTRYINDEX, "USERDATA_METATABLES");
    }
    luau_State(RobloxVMInstance *vm);
    ~luau_State();

    lua_State *get_state() {
        return L;
    }
};

class luau_context { // middle level abstraction
protected:
    lua_State *L;
    luau_State *ls;
    int last_stack_size;
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
    }
    luau_context(luau_State* state, lua_State *thr) {
        ls = state;
        ls->mtx.lock();
        L = thr;
        last_stack_size = lua_gettop(L);
        lua_pushthread(L);
        thr_ref = lua_ref(L, -1);
    }
    luau_context(luau_State *state) {
        ls = state;
        ls->mtx.lock();
        L = state->L;
        last_stack_size = lua_gettop(L);
        lua_pushthread(L);
        thr_ref = lua_ref(L, -1);
    }
    ~luau_context() {
        clear_stack();
        lua_unref(L, thr_ref);
        ls->mtx.unlock();
    }
    inline void expect_empty_stack() {
        last_stack_size = 0;
    }
    inline void dont_clear_stack() {
        last_stack_size = -1;
    }
    inline void push_object() { ::lua_pushnil(L); }
    inline void push_object(std::nullptr_t) { ::lua_pushnil(L); }
    inline void push_object(bool b) { ::lua_pushboolean(L, b); }
    inline void push_object(int64_t integer) { ::lua_pushinteger(L, integer); }
    inline void push_object(double number) { ::lua_pushnumber(L, number); }
    inline void push_object(lua_State *thr) { ::lua_pushthread(thr); ::lua_xmove(thr, L, 1); };
    inline void push_object(const char* str) { ::lua_pushstring(L, str); }
    inline void push_object(const char* str, size_t len) { ::lua_pushlstring(L, str, len); }
    inline void push_object(lua_CFunction f, const char* fname = "<C++ context>") { lua_pushcfunction(L, f, fname); }
    inline void push_object(void* p) {::lua_pushlightuserdata(L, p); }
    inline void push_cclosure(lua_CFunction f, int nup, const char* fname = "<C++ context>") { lua_pushcclosure(L, f, fname, nup); }
    inline void push_object(LuaObject& obj) { obj.get(ls); }
    void push_object(RBXVariant& v);
    inline void push_object(bool b, int idx) { ::lua_pushboolean(L, b); ::lua_insert(L, idx); }
    inline void push_object(int64_t integer, int idx) { ::lua_pushinteger(L, integer); ::lua_insert(L, idx); }
    inline void push_object(double number, int idx) { ::lua_pushnumber(L, number); ::lua_insert(L, idx); }
    inline void push_object(lua_State *thr, int idx) { ::lua_pushthread(thr); ::lua_xmove(thr, L, 1); ::lua_insert(L, idx); };
    inline void push_object(const char* str, int idx) { ::lua_pushstring(L, str); ::lua_insert(L, idx); }
    inline void push_object(const char* str, size_t len, int idx) { ::lua_pushlstring(L, str, len); ::lua_insert(L, idx); }
    inline void push_object(LuaObject& obj, int idx) { obj.get(ls); ::lua_insert(L, idx); }
    inline void push_object(lua_CFunction f, int idx, const char* fname = "<C++ context>") { lua_pushcfunction(L, f, fname); ::lua_insert(L, idx); }
    inline void push_object(void* p, int idx) {::lua_pushlightuserdata(L, p); ::lua_insert(L, idx); }
    void push_object(RBXVariant& v, int idx);
    template <typename T, typename... Others>
    inline int push_objects(T o, Others... others) { push_object(o); return push_objects(others...)+1; }
    template <typename T>
    inline int push_objects(T o) { push_object(o); return 1; }
    inline void push_current_thread() { ::lua_pushthread(L); }
    RBXVariant to_object();
    RBXVariant to_object(int idx);
    RBXVariant as_object();
    RBXVariant as_object(int idx);
    inline lua_State* as_thread(int idx = -1) {return ::lua_tothread(L, idx); }
    inline lua_CFunction as_cfunc(int idx = -1) {return ::lua_tocfunction(L, idx); }
    inline int as_absolute_stack_index(int idx = -1) {return (idx > 0) ? idx : ::lua_gettop(L)+1-idx;}
    inline void push_pointer_hash(int idx) { ::lua_pushinteger(L, (size_t)::lua_topointer(L, idx)); }
    inline int64_t as_pointer_hash(int idx) { return (size_t)::lua_topointer(L, idx); }

    inline int get_type(int idx) { return ::lua_type(L, idx); }
    inline int get_userdata_type(int idx) { return ::lua_userdatatag(L, idx); }
    inline const char* get_typename(int idx) { return ::lua_typename(L, ::lua_type(L, idx)); }
    inline const char* get_usertypename(int idx) {return (lua_type(L, idx) == LUA_TUSERDATA) ? user_types[::lua_userdatatag(L, idx)] : nullptr; }
    inline const char* as_typename(int type) { return ::lua_typename(L, type); }
    inline const char* as_usertypename(int utype) { return user_types[utype]; }
    inline bool is_type(int idx, int type) { return ::lua_type(L, idx) == type; }
    inline bool is_utype(int idx, int utype) { return (is_type(idx,LUA_TUSERDATA)) ? get_userdata_type(idx) == utype : false; }
    inline bool is_cfunction(int idx) { return lua_iscfunction(L, idx); }

    inline void insert_into(int idx) { return ::lua_insert(L, idx); }
    
    template <typename T, typename... Args>
    inline T* new_userdata(int utype, Args... args) {
        T* p = new (::lua_newuserdatatagged(L, sizeof(T), utype)) T(args...);
        rawget(LUA_REGISTRYINDEX,"USERDATA_METATABLES");
        rawget(-1,utype);
        setmetatable(-3);
        pop_stack(1);
        return p;
    }
    template <typename T>
    inline T* new_instance(RobloxVMInstance* vm) {
        static_assert(::std::is_base_of<Instance, T>::value, "To pass a type to new_instance you must pass a class that derives from Instance!");
        T* p = new (::lua_newuserdatatagged(L, sizeof(T), UD_TINSTANCE)) T(vm);
        rawget(LUA_REGISTRYINDEX,"USERDATA_METATABLES");
        rawget(-1,UD_TINSTANCE);
        setmetatable(-3);
        pop_stack(1);
        rawget(LUA_REGISTRYINDEX,"INSTANCE_REFS");
        push_value(-2);
        rawset(-2,(size_t)(void*)p);
        pop_stack(1);
        return p;
    }
    template <typename T>
    inline T* as_userdata(int idx) { //unsafe
        return (T*)::lua_touserdata(L, idx);
    }
    template <typename T>
    inline T* as_userdata(int idx, int utype) { //safe
        return (T*)::lua_touserdatatagged(L, idx, utype);
    }
    inline void set_dtor(int tag, lua_Destructor dtor) { ::lua_setuserdatadtor(L, tag, dtor); }
    inline void setmetatable(int object_idx) {
        ::lua_setmetatable(L, object_idx);
    }
    inline void setmetatable(int object_idx, int metatable_idx) {
        ::lua_pushvalue(L, metatable_idx);
        ::lua_setmetatable(L, (object_idx < 0) ? object_idx-1 : object_idx);
        ::lua_remove(L, metatable_idx);
    }
    inline int getmetatable(int object_idx) {
        return ::lua_getmetatable(L, object_idx);
    }
    inline void newmetatable_type(int utype) {
        get(LUA_REGISTRYINDEX,"USERDATA_METATABLES");
        new_table();
        push_value(-1);
        set(-3);
    }

    inline void freeze(int tbl_idx) {
        ::lua_setreadonly(L, tbl_idx, true);
    }
    inline void unfreeze(int tbl_idx) {
        ::lua_setreadonly(L, tbl_idx, false);
    }
    inline bool is_frozen(int tbl_idx) {
        return ::lua_getreadonly(L, tbl_idx);   
    }

    inline bool has_argument(int argn) { return !lua_isnone(L, argn); }
    inline void assert_has_argument(int argn, const char* argname) {
        if (lua_isnone(L, argn)) 
            errorf("missing argument #%d to '%s' (any expected)", argn, argname);
    }
    inline void assert_type_argument(int argn, const char* argname, int type) {
        if (lua_isnone(L, argn)) 
            errorf("missing argument #%d to '%s' (%s expected)", argn, argname, lua_typename(L, type));
        if (!is_type(argn, type)) 
            errorf("invalid argument #%d to '%s' (%s expected, got %s)", argn, argname, lua_typename(L, type), get_typename(argn));
    }
    inline void assert_usertype_argument(int argn, const char* argname, int utype) {
        if (lua_isnone(L, argn)) 
            errorf("missing argument #%d to '%s' (%s expected)", argn, argname, user_types[utype]);
        if (!lua_isuserdata(L, argn)) 
            errorf("invalid argument #%d to '%s' (%s expected, got %s)", argn, argname, user_types[utype], get_typename(argn));
        if (lua_userdatatag(L, argn) == utype) 
            errorf("invalid argument #%d to '%s' (%s expected, got %s)", argn, argname, user_types[utype], get_typename(argn));
    }
    inline void assert_type_argument(int argn, const char* argname, int type1, int type2) {
        if (lua_isnone(L, argn)) 
            errorf("missing argument #%d to '%s' (%s|%s expected)", argn, argname, lua_typename(L, type1), lua_typename(L, type2));
        if (!is_type(argn, type1) and !is_type(argn, type2)) 
            errorf("invalid argument #%d to '%s' (%s|%s expected, got %s)", argn, argname, lua_typename(L, type1), lua_typename(L, type2), get_typename(argn));
    }

    inline void pop_stack(int n) { lua_pop(L, n); }
    inline void remove_stack(int idx) { lua_remove(L, idx); }
    inline int get_stack_size() { return lua_gettop(L); }
    inline void push_value(int idx) { lua_pushvalue(L, idx); }
    inline void clear_stack() { if (last_stack_size < 0) return; size_t s = last_stack_size - lua_gettop(L); if (s>0) lua_pop(L, s);}

    inline int get(int tbl_idx) { return lua_gettable(L, tbl_idx); }
    inline int get(int tbl_idx, const char* field) { return lua_getfield(L, tbl_idx, field); }
    inline int get(int tbl_idx, int64_t i) { lua_pushnumber(L, i); return lua_gettable(L, (tbl_idx < 0 && !lua_ispseudo(tbl_idx)) ? tbl_idx-1 : tbl_idx); }
    inline void set(int tbl_idx) { lua_settable(L, tbl_idx); }
    inline void set(int tbl_idx, const char* field) { lua_setfield(L, tbl_idx, field); }
    inline void set(int tbl_idx, int64_t i) { lua_pushnumber(L, i); lua_pushvalue(L, -2); lua_settable(L, (tbl_idx < 0 && !lua_ispseudo(tbl_idx)) ? tbl_idx-2 : tbl_idx); }
    inline int rawget(int tbl_idx) { return lua_rawget(L, tbl_idx); }
    inline int rawget(int tbl_idx, const char* field) { return lua_rawgetfield(L, tbl_idx, field); }
    inline int rawget(int tbl_idx, int64_t i) { return lua_rawgeti(L, tbl_idx, i); }
    inline void rawset(int tbl_idx) { lua_rawset(L, tbl_idx); }
    inline void rawset(int tbl_idx, const char* field) { lua_rawsetfield(L, tbl_idx, field); }
    inline void rawset(int tbl_idx, int64_t i) { lua_rawseti(L, tbl_idx, i); }
    inline int len(int idx) { return lua_objlen(L, idx); }

    inline int rawiter(int idx, int iter) { return lua_rawiter(L, idx, iter); }
    inline int next(int idx) { return lua_next(L, idx); }

    inline void new_table() { lua_newtable(L); }
    inline void new_array(int n) { lua_createtable(L, n, 0); }
    inline void new_dictionary(int n) { lua_createtable(L, 0, n); }
    inline void create_array_from_stack(int nargs) { // pops arguments and replaces them with a table
        lua_createtable(L, nargs, 0);
        for (int i = 0; i < nargs; i++) {
            lua_pushvalue(L, -nargs-1+i);
            lua_rawseti(L, -2, i+1);
        }
        for (int i = 0; i < nargs; i++) {
            lua_remove(L, -2);
        }
    }
    inline void clone_table(int tbl_idx = -1) {
        ::lua_pushvalue(L, tbl_idx);
        ::lua_newtable(L);
        int iter = 0;
        while (true) {
            iter = rawiter(-1, iter);
            if (iter == -1) {
                ::lua_remove(L, -2);
                return;
            }
            ::lua_rawset(L, -3);
        }
    }

    inline int getglobal(const char* name) { return lua_rawgetfield(L, LUA_GLOBALSINDEX, name); }
    inline void setglobal(const char* name) { lua_rawsetfield(L, LUA_GLOBALSINDEX, name); }
    inline int getregistry(const char* name) { return lua_rawgetfield(L, LUA_REGISTRYINDEX, name); }
    inline void setregistry(const char* name) { lua_rawsetfield(L, LUA_REGISTRYINDEX, name); }

    inline void call(int nargs, int nres) { lua_call(L, nargs, nres); }
    inline int pcall(int nargs, int nres, int errfuncidx = 0) {
        return lua_pcall(L, nargs, nres, errfuncidx);
    }
    inline double clock() {
        return lua_clock();
    }
    [[noreturn]] inline void error() {
        lua_error(L);
    }
    [[noreturn]] inline void error(const char *s) {
        lua_pushstring(L, s);
        lua_error(L);
    }
    template <typename... T>
    [[noreturn]] inline void errorf(const char *fmt, T... args) {
        lua_pushfstringL(L, fmt, args...);
        lua_error(L);
    }

    inline void yield(int nargs) {
        lua_yield(L, nargs);
    }
    // MAIN: [...], [errfunc], [...], [coro], [args] -> [...], [errfunc], [...], [res]
    // CORO: [...] -> [...]
    // CASE OF ERROR:
    // CORO: [...] -> [...], [error]
    // if handler exists:
    //   MAIN: [...], [errfunc], [...], [coro], [args] -> [...], [errfunc], [...]
    // else
    //   MAIN: [...], [coro], [args] -> [...], [error]
    int resume(int nargs, int nres, int errfuncidx) {
        int stack_size = lua_gettop(L)-nargs;
        int retnres;
        lua_State* thr = lua_tothread(L, -1-nargs);
        if (thr == nullptr) this->errorf("Internal error: expected thread at position %d, got a non-thread object/null.",-1-nargs);
        lua_xmove(L, thr, nargs);
        int status = lua_resume(L, thr, nargs);
        retnres = lua_gettop(thr);
        errfuncidx = (errfuncidx < 0 && !lua_ispseudo(errfuncidx)) ? errfuncidx+nargs : errfuncidx;
        if (status != LUA_OK and status != LUA_YIELD) {
            if (errfuncidx != 0) {
                lua_xpush(L, thr, errfuncidx);
                lua_pushvalue(thr, -2);
                lua_call(thr, 1, 1);
                lua_pop(L, 1); // pop the thread off
            }
        } else {
            lua_pop(L, 1); // pop the thread off
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
    inline lua_State* new_thread(int nargs) { 
        lua_State* thr = lua_newthread(L);
        int ref = lua_ref(L, -1); // create temporary reference and delete the thread so it doesnt get GC'd
        lua_pop(L, 1);
        lua_xmove(L, thr, 1+nargs);
        lua_getref(L, ref);
        lua_unref(L, ref);
        return thr; // if they wanna use it anyways, its on stack
    }
    // [...], [thread] -> [...]
    inline int status() {
        lua_State* thr = lua_tothread(L, -1);
        int status = lua_status(thr);
        lua_pop(L, 1);
        return status;
    }
    // [...] -> [...]
    inline int status(lua_State *thr) {
        return lua_status(thr);
    }
    // [...], [thread] -> [...]
    inline void close() {
        lua_State* thr = lua_tothread(L, -1);
        if (thr != L and lua_mainthread(L) != thr) lua_resetthread(thr);
        lua_pop(L, 1);
    }
    // [...], [thread] -> [...]
    inline void close(lua_State *thr) {
        if (thr != L and lua_mainthread(L) != thr) lua_resetthread(thr);
    }
    // [...], [thread] -> [...]
    inline int costatus() {
        lua_State* thr = lua_tothread(L, -1);
        int status = lua_costatus(L, thr);
        lua_pop(L, 1);
        return status;
    }
    // [...] -> [...]
    inline int costatus(lua_State *thr) {
        return lua_costatus(L, thr);
    }
    inline bool can_yield() {
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
    [[noreturn]] inline void dbg_break() { ::lua_break(L); }

    inline const char* getlocal(int level, int n) {
        return lua_getlocal(L, level, n);
    }
    inline void getfenv(int funcidx) { lua_getfenv(L, funcidx); }
    inline void setfenv(int funcidx) { lua_setfenv(L, funcidx); }
    inline const char* getupvalue(int funcidx, int n) { return lua_getupvalue(L, funcidx, n); }
    inline const char* getupvalue_level(int level, int n) { 
        delete getinfo("f",level);
        const char *ret = lua_getupvalue(L, -1, n);
        lua_remove(L, (ret == nullptr) ? -1 : -2);
        return ret;
    }
    inline const char* setupvalue(int funcidx, int n) { return lua_setupvalue(L, funcidx, n); }
    inline const char* setupvalue_level(int level, int n) { 
        delete getinfo("f",level);
        const char *ret = lua_setupvalue(L, -1, n);
        lua_remove(L, (ret == nullptr) ? -1 : -2);
        return ret;
    }
    inline void getfenv_level(int level) { 
        delete getinfo("f",level);
        lua_getfenv(L, -1);
        lua_remove(L, -2);
    }
    inline void setfenv_level(int level) { 
        delete getinfo("f",level);
        lua_insert(L, -2);
        lua_setfenv(L, -2); 
        lua_remove(L, -1);
    }
    

    inline int new_ref(int idx) { return lua_ref(L, idx); }
    inline void push_ref(int ref) { lua_getref(L, ref); }
    inline void delete_ref(int ref) { lua_unref(L, ref); }

    inline void move_args(lua_State *to, int amount) {::lua_xmove(L, to, amount);}
    inline void copy_arg(lua_State *to, int idx) {::lua_xpush(L, to, idx); }

    inline bool rawequal(int idx1, int idx2) { return ::lua_rawequal(L, idx1, idx2); }
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
    RBXVariant(LuaObject& o) : type(Type::RBXVARIANT_OBJ), obj(o) {}
    RBXVariant(const char *s) : type(Type::RBXVARIANT_STR) {
        strl = strlen(s)+1;
        str = new char[strl];
        strcpy(str, s);
    }
    RBXVariant(const char* s, size_t l) : type(Type::RBXVARIANT_STR) {
        strl = l+1;
        str = new char[strl];
        memcpy(str, s, strl);
    }
    RBXVariant(void* p) : type(Type::RBXVARIANT_PTR), ptr(p) {}
    RBXVariant(RBXVariant& other) : type(other.type) {
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
            memcpy(str,other.str,strl);
            break;
        case Type::RBXVARIANT_PTR:
            ptr = other.ptr;
        default:
            break;
        }
    }
    ~RBXVariant() {
        switch (type) {
        case Type::RBXVARIANT_OBJ:
            obj.~LuaObject();
            break;
        case Type::RBXVARIANT_STR:
            delete str;
            break;
        default:
            break;
        }
    }
    operator bool() {
        switch (type) {
        case Type::RBXVARIANT_NIL: return false;
        case Type::RBXVARIANT_NUM: return number > 0;
        case Type::RBXVARIANT_BOOL: return boolean;
        case Type::RBXVARIANT_INT: return integer != 0;
        default: return true;
        }
    }
    operator int64_t() {
        switch (type) {
        case Type::RBXVARIANT_NIL: return 0;
        case Type::RBXVARIANT_NUM: return (int)number;
        case Type::RBXVARIANT_BOOL: return boolean;
        case Type::RBXVARIANT_INT: return integer;
        case Type::RBXVARIANT_PTR: return (size_t)ptr;
        default: return 1;
        }
    }
    operator double() {
        switch (type) {
        case Type::RBXVARIANT_NIL: return 0.0;
        case Type::RBXVARIANT_NUM: return number;
        case Type::RBXVARIANT_BOOL: return (double)boolean;
        case Type::RBXVARIANT_INT: return integer;
        default: return NAN;
        }
    }
    operator LuaObject() {
        return obj;
    }
    operator LuaObject&() {
        return obj;
    }
    int get_slen() {
        return strl;
    }
    const char *get_str() {
        return str;
    }
    operator void*() {
        return ptr;
    }
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
    lua_State* create_lua_state() {
        lua_State *L = lua_newstate(lua_alloc, nullptr);
        ::lua_pushlightuserdata(L, this);
        ::lua_rawsetfield(L, LUA_REGISTRYINDEX, "ROBLOX_VM");
        register_types(L);
        return L;
    }
public:
    RobloxVMInstance(lua_State* main);
    ~RobloxVMInstance();

    luau_State* main_synchronized;
    Instance* game;
    
    //SERVICES


    //misc

};

}

#endif