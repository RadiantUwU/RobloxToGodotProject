#ifndef HLLC_HASH_MAP
#define HLLC_HASH_MAP
#include <algorithm>
#include <memory>
#ifdef USE_GODOT
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/pair.hpp>
#else
#include <unordered_map>
#include <iterator>
#endif
#include <initializer_list>
#include "defs.hpp"
namespace HLLC::_LLC {
template <typename K, typename V>
struct KeyValuePair {
#ifdef USE_GODOT
    KeyValuePair(::godot::KeyValue<K, V>& pair) : key(pair.key), value(pair.value) {}
    KeyValuePair(const ::godot::KeyValue<K, V>& pair) : key(pair.key), value(pair.value) {}
#else
    KeyValuePair(::std::pair<K, V>& pair) : key(pair.first), value(pair.second) {}
    KeyValuePair(const ::std::pair<K, V>& pair) : key(pair.first), value(pair.second) {}
#endif
public:
    const K& key;
    V& value;
};
template <typename K, typename V>
class HashMap {
#ifdef USE_GODOT
    ::godot::HashMap<K, V> map;
#else
    ::std::unordered_map<K, V> map;
#endif
public:
#ifdef USE_GODOT // Godot defs
    //Iterators
    HLLC_INLINE auto begin() { map.begin(); }
    HLLC_INLINE auto begin() const { map.begin(); }
    HLLC_INLINE auto cbegin() const { map.begin(); }
    HLLC_INLINE auto cend() const { map.end(); }
    HLLC_INLINE auto end() { map.end(); }
    HLLC_INLINE auto end() const { map.end(); }

    //Read operations
    HLLC_INLINE const K& find(const V& val, const K& not_found_key) const { 
        for (auto& kv : map) {
            if (kv.value == val) return kv.key;
        }
        return not_found_key;
    }
    HLLC_INLINE bool is_empty() const { return map.is_empty(); }
    HLLC_INLINE size_t size() const { return map.size(); }
    HLLC_INLINE bool has(const K& o) { return map.has(o); }

    // Write operations
    HLLC_INLINE void merge(const HashMap<K, V>& o, bool overload = false) {
        size_t old_size = size();
        map.reserve(old_size+o.size()); // maximum size
        for (auto& kv : o) {
            if (overload || !has(kv.key)) {
                *this[kv.key] = kv.value;
            }
        }
    }
    HLLC_INLINE void clear() { map.clear(); }
    HLLC_INLINE void erase(const K& k) { map.erase(k); } // Remove if exists

    //Misc operations
    HLLC_INLINE V& operator[](K& key) {
        return map[key];
    }
    HLLC_INLINE const V& operator[](K& key) const {
        return map[key];
    }
    HLLC_INLINE HashMap<K, V> duplicate() const { HashMap<K, V> v; v.map = map; return v; }

#else // C++ std defs
#error "ZONE UNDER CONSTRUCTION, hash_map.hpp case USE_GODOT = false"
    //Iterators
    HLLC_INLINE auto begin() { vect.begin(); }
    HLLC_INLINE auto begin() const { vect.cbegin(); }
    HLLC_INLINE auto cbegin() const { vect.cbegin(); }
    HLLC_INLINE auto cend() const { vect.cend(); }
    HLLC_INLINE auto crbegin() const { vect.crbegin(); }
    HLLC_INLINE auto crend() const { vect.crend(); }
    HLLC_INLINE auto end() { vect.end(); }
    HLLC_INLINE auto end() const { vect.cend(); }
    HLLC_INLINE auto rbegin() { vect.rbegin(); }
    HLLC_INLINE auto rbegin() const { vect.rbegin(); }
    HLLC_INLINE auto rend() { vect.rend(); }
    HLLC_INLINE auto rend() const { vect.crend(); }

    //To C array
    HLLC_INLINE const T* ptr() const { return ::std::addressof(*vect.cbegin()); }
    HLLC_INLINE T* ptrw() { return ::std::addressof(*vect.begin()); }

    //Read operations
    HLLC_INLINE size_t find(const T& val, size_t from = 0) const { 
        assert(from < size()); 
        return ::std::find(from+vect.cbegin(),vect.cend(),val); 
    }
    HLLC_INLINE bool is_empty() const { return vect.empty(); }
    HLLC_INLINE size_t size() const { return vect.size(); }
    HLLC_INLINE bool has(const T& o) { return find(o) != cend(); }

    // Write operations
    HLLC_INLINE void append(const T& o) { vect.push_back(o); }
    HLLC_INLINE void append_array(const Vector<K, V>& o) {
        size_t old_size = size();
        resize(old_size+o.size());
        auto self_iter = begin()+old_size;
        for (const T& i : o) {
            *self_iter = i;
            self_iter++;
        }
    }
    HLLC_INLINE void clear() { vect.clear(); }
    HLLC_INLINE void erase(const T& o) { vect.erase(o); } // Remove if exists
    HLLC_INLINE void insert(size_t index, const T& o) { vect.insert(index, o); }
    HLLC_INLINE void resize(size_t new_size) { vect.resize(new_size); }
    HLLC_INLINE void sort() { std::sort(begin(),end()); }
    template <class C>
    HLLC_INLINE void sort_custom() { std::sort(begin(),end(),C()); }

    //Misc operations
    HLLC_INLINE T& operator[](size_t index) {
        return vect.at(index);
    }
    HLLC_INLINE const T& operator[](size_t index) const {
        return vect.at(index);
    }
    HLLC_INLINE T& back() {assert_msg(!is_empty(),"Vector is empty."); return vect.back();}
    HLLC_INLINE const T& back() const {assert_msg(!is_empty(),"Vector is empty."); return vect.back();}
    HLLC_INLINE Vector<K, V> duplicate() const { Vector<T> v; v.vect = vect; return v; }
    HLLC_INLINE T& front() {assert_msg(!is_empty(),"Vector is empty."); return vect.front();}
    HLLC_INLINE const T& front() const {assert_msg(!is_empty(),"Vector is empty."); return vect.front();}
    HLLC_INLINE void reverse() { std::reverse(begin(), end()); }

#endif // Common defs
    

};
} // namespace HLLC::_LLC
#endif