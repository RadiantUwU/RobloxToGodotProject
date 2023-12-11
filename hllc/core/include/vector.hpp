#ifndef HLLC_VECTOR
#define HLLC_VECTOR
#include <algorithm>
#include <memory>
#ifdef USE_GODOT
#include <godot_cpp/templates/vector.hpp>
#else
#include <vector>
#endif
#include <initializer_list>
#include "defs.hpp"
namespace HLLC::_LLC {
template <typename T>
class Vector {
#ifdef USE_GODOT
    ::godot::Vector<T> vect;
#else
    ::std::vector<T> vect;
#endif
public:
    Vector() {};
    Vector(std::initializer_list<T> list) : vect(list) {};
#ifdef USE_GODOT // Godot defs
    //Iterators
    HLLC_INLINE auto begin() { vect.begin(); }
    HLLC_INLINE auto begin() const { vect.begin(); }
    HLLC_INLINE auto cbegin() const { vect.begin(); }
    HLLC_INLINE auto cend() const { vect.end(); }
    HLLC_INLINE auto end() { vect.end(); }
    HLLC_INLINE auto end() const { vect.end(); }

    //To C array
    HLLC_INLINE const T* ptr() const { return vect.ptr(); }
    HLLC_INLINE T* ptrw() { return vect.ptrw(); }

    //Read operations
    HLLC_INLINE size_t find(const T& val, size_t from = 0) const { return vect.find(val, from); }
    HLLC_INLINE bool is_empty() const { return vect.is_empty(); }
    HLLC_INLINE size_t size() const { return vect.size(); }
    HLLC_INLINE bool has(const T& o) { return vect.has(o); }

    // Write operations
    HLLC_INLINE void append(const T& o) { vect.append(o); }
    HLLC_INLINE void append_array(const Vector<T>& o) {
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
    HLLC_INLINE const T& pop() { assert_msg(!is_empty(),"Vector is empty."); const T obj = back(); resize(size()-1); return obj;}
    HLLC_INLINE void resize(size_t new_size) { vect.resize(new_size); }
    HLLC_INLINE void sort() { vect.sort(); }
    template <class C>
    HLLC_INLINE void sort_custom() { vect.sort_custom<C>(); }

    //Misc operations
    HLLC_INLINE T& operator[](size_t index) {
        index = ((index < 0) ? size()+index : index);
        return vect.get(index);
    }
    HLLC_INLINE const T& operator[](size_t index) const {
        index = ((index < 0) ? size()+index : index);
        return vect.get(index);
    }
    HLLC_INLINE T& back() {assert_msg(!is_empty(),"Vector is empty."); return vect[size()-1];}
    HLLC_INLINE const T& back() const {assert_msg(!is_empty(),"Vector is empty."); return vect[size()-1];}
    HLLC_INLINE Vector<T> duplicate() const { Vector<T> v; v.vect = vect; return v; }
    HLLC_INLINE T& front() {assert_msg(!is_empty(),"Vector is empty."); return vect[0];}
    HLLC_INLINE const T& front() const {assert_msg(!is_empty(),"Vector is empty."); return vect[0];}
    HLLC_INLINE void reverse() { vect.reverse(); }

#else // C++ std defs
    //Iterators
    HLLC_INLINE auto begin() { vect.begin(); }
    HLLC_INLINE auto begin() const { vect.cbegin(); }
    HLLC_INLINE auto cbegin() const { vect.cbegin(); }
    HLLC_INLINE auto cend() const { vect.cend(); }
    HLLC_INLINE auto end() { vect.end(); }
    HLLC_INLINE auto end() const { vect.cend(); }

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
    HLLC_INLINE void append_array(const Vector<T>& o) {
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
    HLLC_INLINE Vector<T> duplicate() const { Vector<T> v; v.vect = vect; return v; }
    HLLC_INLINE T& front() {assert_msg(!is_empty(),"Vector is empty."); return vect.front();}
    HLLC_INLINE const T& front() const {assert_msg(!is_empty(),"Vector is empty."); return vect.front();}
    HLLC_INLINE void reverse() { std::reverse(begin(), end()); }

#endif // Common defs
    

};
} // namespace HLLC::_LLC

#endif