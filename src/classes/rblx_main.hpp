#ifndef RBLX_MAIN
#define RBLX_MAIN

#include <cstring>
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
        ::lua_pushlightuserdata(L, VM);
        ::lua_rawsetfield(L, LUA_REGISTRYINDEX, "ROBLOX_VM");
        ::lua_pushlightuserdata(L, this);
        ::lua_rawsetfield(L, LUA_REGISTRYINDEX, "LUAU_STATE");
    }
    luau_State(RobloxVMInstance *vm);
    ~luau_State();
};

class luau_context { // middle level abstraction
protected:
    lua_State *L;
    luau_State *ls;
    int last_stack_size;
    int thr_ref;
    friend class LuaObject;
public:
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
    inline void push_object(bool b) { ::lua_pushboolean(L, b); }
    inline void push_object(long long integer) { ::lua_pushinteger(L, integer); }
    inline void push_object(double number) { ::lua_pushnumber(L, number); }
    inline void push_object(lua_State *thr) { ::lua_pushthread(thr); ::lua_xmove(thr, L, 1); };
    inline void push_object(const char* str) { ::lua_pushstring(L, str); }
    inline void push_object(const char* str, size_t len) { ::lua_pushlstring(L, str, len); }
    inline void push_object(lua_CFunction f) { ::lua_pushcfunction(L, f); }
    inline void push_object(LuaObject obj) { obj.get(ls); }
    inline void push_object(RBXVariant& v);
    inline void push_object(int idx) { ::lua_pushnil(L); ::lua_insert(L, idx); }
    inline void push_object(bool b, int idx) { ::lua_pushboolean(L, b); ::lua_insert(L, idx); }
    inline void push_object(long long integer, int idx) { ::lua_pushinteger(L, integer); ::lua_insert(L, idx); }
    inline void push_object(double number, int idx) { ::lua_pushnumber(L, number); ::lua_insert(L, idx); }
    inline void push_object(lua_State *thr, int idx) { ::lua_pushthread(thr); ::lua_xmove(thr, L, 1); ::lua_insert(L, idx); };
    inline void push_object(const char* str, int idx) { ::lua_pushstring(L, str); ::lua_insert(L, idx); }
    inline void push_object(const char* str, size_t len, int idx) { ::lua_pushlstring(L, str, len); ::lua_insert(L, idx); }
    inline void push_object(LuaObject obj, int idx) { obj.get(ls); ::lua_insert(L, idx); }
    inline void push_object(lua_CFunction f, int idx) { ::lua_pushcfunction(L, f); ::lua_insert(L, idx); }
    inline void push_object(RBXVariant& v, int idx);
    inline RBXVariant to_object();
    inline RBXVariant to_object(int idx);
    inline RBXVariant as_object();
    inline RBXVariant as_object(int idx);
    inline lua_State* as_thread(int idx = -1) {return ::lua_tothread(L, idx); }
    inline lua_CFunction as_cfunc(int idx = -1) {return ::lua_tocfunction(L, idx); }
    inline int as_absolute_stack_index(int idx = -1) {return (idx > 0) ? idx : ::lua_gettop(L)+1-idx;}

    inline int get_type(int idx) { return ::lua_type(L, idx); }
    inline const char* get_typename(int idx) { return ::lua_typename(L, ::lua_type(L, idx)); }
    inline const char* as_typename(int type) { return ::lua_typename(L, type); }
    inline bool is_type(int idx, int type) { return ::lua_type(L, idx) == type; }
    inline bool is_cfunction(int idx) { return lua_iscfunction(L, idx); }

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

    inline void get(int tbl_idx) { lua_gettable(L, tbl_idx); }
    inline void get(int tbl_idx, const char* field) { lua_getfield(L, tbl_idx, field); }
    inline void get(int tbl_idx, long long i) { lua_pushnumber(L, i); lua_gettable(L, (tbl_idx < 0 && !lua_ispseudo(tbl_idx)) ? tbl_idx-1 : tbl_idx); }
    inline void set(int tbl_idx) { lua_settable(L, tbl_idx); }
    inline void set(int tbl_idx, const char* field) { lua_setfield(L, tbl_idx, field); }
    inline void set(int tbl_idx, long long i) { lua_pushnumber(L, i); lua_pushvalue(L, -2); lua_settable(L, (tbl_idx < 0 && !lua_ispseudo(tbl_idx)) ? tbl_idx-2 : tbl_idx); }
    inline void rawget(int tbl_idx) { lua_rawget(L, tbl_idx); }
    inline void rawget(int tbl_idx, const char* field) { lua_rawgetfield(L, tbl_idx, field); }
    inline void rawget(int tbl_idx, long long i) { lua_rawgeti(L, tbl_idx, i); }
    inline void rawset(int tbl_idx) { lua_rawset(L, tbl_idx); }
    inline void rawset(int tbl_idx, const char* field) { lua_rawsetfield(L, tbl_idx, field); }
    inline void rawset(int tbl_idx, long long i) { lua_rawseti(L, tbl_idx, i); }
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

    inline void getglobal(const char* name) { lua_rawgetfield(L, LUA_GLOBALSINDEX, name); }
    inline void setglobal(const char* name) { lua_rawsetfield(L, LUA_GLOBALSINDEX, name); }
    inline void getregistry(const char* name) { lua_rawgetfield(L, LUA_REGISTRYINDEX, name); }
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
                lua_pop(L, retnres-nres)
            }
        }
        return status;
    }
    // [...], [Function], [nargs] -> [...], [thread]
    inline lua_State* create(int nargs) { 
        lua_State* thr = lua_newthread(L);
        int ref = lua_ref(L, -1); // create temporary reference and delete the thread so it doesnt get GC'd
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
        if (!lua_getinfo(L, what, d)) {
            delete d;
            return nullptr;
        }
        return d;
    }
    lua_Debug* getinfo(const char* what, int level) {
        lua_Debug* d = new lua_Debug;
        if (!lua_getstack(L, level, d)) {
            delete d;
            return nullptr;
        }
        if (!lua_getinfo(L, what, d)) {
            delete d;
            return nullptr;
        }
        return d;
    }
    inline lua_Hook gethook() { return ::lua_gethook(L); }
    inline int gethookcount() { return ::lua_gethookcount(L); }
    inline int gethookmask() { return ::lua_gethookmask(L); }
    [[noreturn]] inline void dbg_break() { ::lua_break(L); }

    inline const char* getlocal(int level, int n) {
        lua_Debug d;
        if (!lua_getstack(L, level, &d)) return nullptr;
        return lua_getlocal(L, &d, n);
    }
    inline const char* getfuncargname(int n) { return lua_getlocal(L, nullptr, n); }
    inline const char* getupvalue(int funcidx, int n) { return lua_getupvalue(L, funcidx, n); }
    inline void getfenv(int funcidx) { lua_getfenv(L, funcidx); }
    inline void setfenv(int funcidx) { lua_setfenv(L, funcidx); }

    inline int new_ref(int idx) { return lua_ref(L, idx); }
    inline void push_ref(int ref) { return lua_getref(L, ref); }
    inline void delete_ref(int ref) { return lua_unref(L, ref); }

    inline void move_args(lua_State *to, int amount) {::lua_xmove(L, to, amount);}
    inline void copy_arg(lua_State *to, int idx) {::lua_xpush(L, to, idx); }

    inline bool rawequal(int idx1, int idx2) { return ::lua_rawequal(L, idx1, idx2); }
};

struct RBXVariant final {
private:
    union {
        int integer = 0;
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
        RBXVARAINT_NUM,
        RBXVARIANT_OBJ,
        RBXVARIANT_STR,
        RBXVARIANT_PTR
    } type = Type::RBXVARIANT_NIL;
    RBXVariant() : type(Type::RBXVARIANT_NIL) {}
    RBXVariant(bool b) : type(Type::RBXVARIANT_BOOL), boolean(b) {}
    RBXVariant(int i) : type(Type::RBXVARIANT_INT), integer(i) {}
    RBXVariant(double n) : type(Type::RBXVARAINT_NUM), number(n) {}
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
        case Type::RBXVARAINT_NUM: return number > 0;
        case Type::RBXVARIANT_BOOL: return boolean;
        case Type::RBXVARIANT_INT: return integer != 0;
        default: return true;
        }
    }
    operator int() {
        switch (type) {
        case Type::RBXVARIANT_NIL: return 0;
        case Type::RBXVARAINT_NUM: return (int)number;
        case Type::RBXVARIANT_BOOL: return boolean;
        case Type::RBXVARIANT_INT: return integer;
        case Type::RBXVARIANT_PTR: return (size_t)ptr;
        default: return 1;
        }
    }
    operator double() {
        switch (type) {
        case Type::RBXVARIANT_NIL: return 0.0;
        case Type::RBXVARAINT_NUM: return number;
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

    lua_State* create_lua_state() {
        return lua_newstate(lua_alloc, nullptr);
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