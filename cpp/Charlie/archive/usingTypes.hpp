#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include "linearMem.hpp"


/**
 * This file declares abbreviations for some of the most frequently used 
 * STL containers. Additionally, if `USE_LINEAR_MEM` is set to true, the 
 * type names in this file will refer to containers loaded with the linear 
 * memory allocator.
 * 
 * Intended usage: in the main file, write
 * 
 *     #define USE_LINEAR_MEM // Comment this line to the original STL containers.
 *     #include "usingTypes.hpp"
 *     ...
 *        End of file
 *        
 *     #ifdef USE_LINEAR_MEM
 *     #undef USE_LINEAR_MEM
 *     #endif
 *  
 * Keep adding new containers as we go.
 */


#ifdef USE_LINEAR_MEM


// =============================================================================
// vector <==> vec
// =============================================================================
template <typename T, typename V = std::vector<T, LinearMem::Pool<T> > >
struct vec: public V
{
  using V::V; // In order to inherit the base class's constructors.


  /**
   * Ensure the vector calls the destructor in reverse order of the elements.
   * 
   * This will not always be the optimal order of deallocation in linear
   * memory. 
   * 
   * Calling destructor of derived class will auto call base class's
   * destructor.
   */
  ~vec()
  {
    if constexpr ( !std::is_trivially_destructible<T>::value )
    {
      for (auto it = this->rbegin(); it != this->rend(); it->~T(), ++it);
    }
  }
};
// template <typename T>
// using vec = std::vector<T, LinearMem::Pool<T> >;


// =============================================================================
// string <==> str
// =============================================================================
using str = std::basic_string<
  char, 
  std::char_traits<char>, 
  LinearMem::Pool<char>
>;




// =============================================================================
// Extend std::hash and std::equal_to classes in std::
// =============================================================================
namespace std {
template <> struct hash<str>
{
  std::size_t operator()(const str & x) const
  {
    auto y = std::string_view (&x[0], x.size());
    return std::hash<std::string_view>()(y);
  } 
};
template <> struct equal_to<str>
{
  bool operator()(const str & x, const str & y) const
  {
    if (x.size() != y.size()) return false;
    for (auto xi = x.begin(), yi = y.begin(); xi != x.end(); ++xi, ++yi)
    {  
      if ( *xi != *yi ) return false; 
    } 
    return true;
  } 
};
}


// =============================================================================
// Unordered_map <==> hashmap
// =============================================================================
template <typename Key, 
          typename T, 
          typename Hash = std::hash<Key>, 
          typename KeyEqual = std::equal_to<Key> > 
using hashmap = std::unordered_map <
  Key, T, 
  Hash, 
  KeyEqual, 
  LinearMem::Pool<std::pair<const Key, T> >
>;




// =============================================================================
// Unordered_set <==> hashset
// =============================================================================
template <typename Key, 
          typename Hash = std::hash<Key>, 
          typename KeyEqual = std::equal_to<Key> > 
using hashset = std::unordered_set <
  Key,
  Hash, 
  KeyEqual, 
  LinearMem::Pool< Key >
>;




// #############################################################################
// Using STL containers.
// #############################################################################
#else 



#endif













