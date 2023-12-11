#include <cassert>
#include <cstddef>
#include <lua.h>
#include "hash_map.hpp"
#include "string.hpp"
#include <lualib.h>
#include "lua_misc.hpp"
#include <mutex>
#include <type_traits>

#ifndef HLLC_LLC
#define HLLC_LLC

namespace HLLC::_LLC {
    using mutex = ::std::recursive_mutex;
    using lock_guard = ::std::lock_guard<mutex>;
    template <typename, typename = void>
    struct has_lua_destructor : ::std::false_type {};
    template <typename T>
    struct has_lua_destructor<T, std::void_t<decltype(::std::declval<T>().lua_destroy(std::declval<lua_State*>()))>> : ::std::true_type {};
    class LuauLowLevelState {
    private:
        size_t type_counter = 0x100;
        HashMap<size_t, size_t> type_refs;
        HashMap<size_t, String> type_ids;
        HashMap<size_t, size_t> type_hashes;
        template <typename T, typename DerivedT = T, typename... Args>
        T* new_userdata(lua_State *thr, Args... args) {
            static_assert(::std::is_base_of_v<T, DerivedT>);
            assert_msg(type_hashes.has(typeid(T).hash_code()),"Type is not registered in the low level state.");
            T* obj = new (lua_newuserdatatagged(thr, sizeof(DerivedT), type_hashes[typeid(T).hash_code()])) DerivedT(args...);
            lua_getref(thr, type_refs[typeid(T).hash_code()]);
            lua_setmetatable(thr, -2);
            return obj;
        }
        template <typename T, typename DerivedT = T>
        T* new_userdata(lua_State *thr) {
            static_assert(::std::is_base_of_v<T, DerivedT>);
            assert_msg(type_hashes.has(typeid(T).hash_code()),"Type is not registered in the low level state.");
            T* obj = new (lua_newuserdatatagged(thr, sizeof(DerivedT), type_hashes[typeid(T).hash_code()])) DerivedT;
            lua_getref(thr, type_refs[typeid(T).hash_code()]);
            lua_setmetatable(thr, -2);
            return obj;
        }
    protected:
        friend class LuauLowLevelContext;
        mutex mtx;
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
        lua_State *main = nullptr;
        HLLC_INLINE bool is_initialized() {
            if (!has_vm()) return false;
            lock_guard m(mtx);
            lua_rawgetfield(main, LUA_REGISTRYINDEX, "HLLC_STATE");
            bool is_init = !lua_isnil(main, -1);
            lua_pop(main, 1);
            return is_init;
        }
        void _initialize();
        virtual void on_initialize();
        void initialize();
    public:
        LuauLowLevelState() {
            main = lua_newstate(lua_alloc, nullptr);
            on_initialize();
        };
        LuauLowLevelState(const LuauLowLevelState& o);
        LuauLowLevelState(::std::nullptr_t) {};
        LuauLowLevelState(lua_State *vm) {
            main = vm;
            assert(!is_initialized());
            initialize();
        }
        HLLC_INLINE bool has_vm() { return main != nullptr; }
        HLLC_INLINE lua_State* get_vm() { return main; }
        HLLC_INLINE void set_vm(lua_State *vm) {
            assert(!is_initialized());
            main = vm;
            assert(!is_initialized());
            initialize();
        }
        static LuauLowLevelState* get_state(lua_State *L) {
            lua_rawgetfield(L, LUA_REGISTRYINDEX, "HLLC_STATE");
            LuauLowLevelState* state = (LuauLowLevelState*)lua_tolightuserdata(L, -1);
            lua_pop(L, 1);
            return state;
        }
        template <typename T>
        HLLC_INLINE void register_type() {
            register_type<T>(type_counter++);
        }
        template <typename T>
        HLLC_INLINE void register_type(String name) {
            register_type<T>(type_counter++, name);
        }
        template <typename T>
        HLLC_INLINE void register_type(size_t id) {
            register_type<T>(id, typeid(T).name());
        }
        template <typename T>
        HLLC_INLINE void register_type(size_t id, String name) {
            assert_msg(has_vm(),"Luau VM not assigned to low level luau state.");
            type_refs[id] = lua_ref(L, -1);
            type_hashes[typeid(T).hash_code()] = id;
            type_ids[id] = name;
            lua_Destructor destructor;
            if (has_lua_destructor<T>) {
                destructor = [](lua_State *L,void* ud){(T*)ud->lua_destroy(L);};
            }
        }
        template <typename T>
        HLLC_INLINE bool is_type_registered() {
            return type_hashes.has(typeid(T).hash_code());
        }
    };
    class LuauLowLevelContext {
    protected:
        lua_State *L;
        LuauLowLevelState* vm;
        size_t last_stack_size;
        HLLC_INLINE size_t dont_clear_stack() {
            last_stack_size = 0;
        }
    public:
        LuauLowLevelContext(lua_State *L) {
            this->L = L;
            vm = LuauLowLevelState::get_state(L);
            assert_msg(vm != nullptr,"Attempt to use LuauLowLevelContext with an uninitialized Luau VM (did you perhaps forget to run LuauLowLevelState.set_vm() on it?)")
            vm->mtx.lock();
        }
        LuauLowLevelContext(LuauLowLevelState* vm) {
            this->vm = vm;
            L = vm->get_vm();
            assert_msg(L != nullptr,"Attempt to use LuauLowLevelContext with an uninitialized Luau VM (did you perhaps forget to run LuauLowLevelState.set_vm() on it?)")
            vm->mtx.lock();
        }
        ~LuauLowLevelContext() {
            clear_stack();
            vm->mtx.unlock();
        }
        // Often used for locals!
        HLLC_INLINE size_t get_stack_position_absolute(size_t idx) { return (idx < 0) ? lua_gettop(L)+idx+1 : idx; }
        HLLC_INLINE size_t clear_stack() {
            if (last_stack_size != 0 && lua_gettop(L) > last_stack_size) {
                lua_settop(L, last_stack_size);
            }
        }

        HLLC_INLINE void push_object() { lua_pushnil(L); }
        HLLC_INLINE void push_object(double n) { lua_pushnumber(L, n); }
        HLLC_INLINE void push_object(const char* c) { lua_pushliteral(L, c); }
        HLLC_INLINE void push_object(const char* c, size_t l) { lua_pushlstring(L, c, l); }
        HLLC_INLINE void push_object(const String& s) { lua_pushlstring(L, s.c, s.l); }
        HLLC_INLINE void push_object(const LuaObjectWrapper& o) { lua_getref(L, o.get_ref()); }
        HLLC_INLINE void push_object(lua_State *thr) { lua_pushthread(thr); lua_xmove(thr, L, 1); }
        HLLC_INLINE void push_object(const LuaVariantWrapper& v) {
            switch (v.get_type()) {
            case LuaVariantWrapper::TYPE_NIL:
                push_object();
                break;
            case LuaVariantWrapper::TYPE_LUA_OBJECT:
                push_object(v.cast<LuaObjectWrapper>());
                break;
            case LuaVariantWrapper::TYPE_NUMBER:
                push_object(v.cast<double>());
                break;
            case LuaVariantWrapper::TYPE_STRING:
                push_object(v.cast<String>());
                break;
            case LuaVariantWrapper::TYPE_CFUNC:
                push_object(v.cast<lua_CFunction>());
                break;
            case LuaVariantWrapper::TYPE_LIGHTUSERDATA:
#ifdef HLLC_ENABLE_CROSS_STATE_OBJECTS
                get_registry("HLLC_")
#else
                push_object(v.cast_ud_const());
                get_registry("HLLC_")
#endif
                break;
            }
        }

        HLLC_INLINE void push_current_thread() { lua_pushthread(L); }
    };
}

#endif