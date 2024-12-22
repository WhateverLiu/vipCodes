#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include "linearMem.hpp"


/**
 * This file declares abbreviations for some of the most frequently used 
 * STL containers. The type names in this file will refer to 
 * containers loaded with the linear memory allocator.
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


namespace Allc {


// =============================================================================
// vector <==> vec
// =============================================================================
template <typename T>
using vec = std::vector<T, LinearMem::Pool<T> >;


// =============================================================================
// string <==> str
// =============================================================================
using str = std::basic_string<
  char, 
  std::char_traits<char>, 
  LinearMem::Pool<char>
>; 




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




struct activate
{
  LinearMem::activate * at;
  activate(activate const &) = delete;
  void operator=(activate const &) = delete;
  activate(
    int && maxCore = 1,
	bool verbose = false,
    std::size_t bucketSize = 64, // MB
    std::size_t workerBucketSize = 13, // MB
    char bucketSizeUnit = 'M')
  { 
    at = new LinearMem::activate (
      verbose, std::move(maxCore), bucketSize,
      workerBucketSize, bucketSizeUnit);
  }
  ~activate () { delete at; }
}; 




template <typename ... Ts>
void linearize ( Ts & ... objs ) 
{
  LinearMem::linearize ( objs ... );
};


}




// =============================================================================
// Extend std::hash and std::equal_to classes in std::
// =============================================================================
namespace std {
template <> struct hash<Allc::str>
{
  std::size_t operator()(const Allc::str & x) const
  {
    auto y = std::string_view (&x[0], x.size());
    return std::hash<std::string_view>()(y);
  }  
};
template <> struct equal_to<Allc::str>
{
  bool operator()(const Allc::str & x, const Allc::str & y) const
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






















