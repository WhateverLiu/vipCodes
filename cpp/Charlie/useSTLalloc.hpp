#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "parFor.hpp"


namespace Allc {


/**
 * Allocator adaptor that interposes construct() calls to
 * convert value initialization into default initialization.
 */
template < typename T >
struct StlAlloc : public std::allocator<T>
{
  /**
   * Here the new operator are placement new. It will not call `malloc` or `new`
   * if U is default constructible.
   */
  template <typename U, typename... Args>
  void construct(U* ptr, Args &&... args)
  {
    if constexpr (std::is_nothrow_default_constructible<U>::value and 
                    sizeof...(args) == 0)
      ::new(static_cast<void*>(ptr)) U;
    else 
      ::new(static_cast<void*>(ptr)) U(std::forward<Args>(args)...);
  } 
  
};
  
  
  

// =============================================================================
// Vector <==> vec
// =============================================================================
template<typename T, typename A = StlAlloc<T> >
using vec = std::vector<T, A>;


// =============================================================================
// str <==> std::string
// =============================================================================
using str = std::basic_string<char, std::char_traits<char>, StlAlloc<char> >;


// =============================================================================
// Unordered_map <==> hashmap
// =============================================================================
template < typename Key, 
           typename T, 
           typename Hash = std::hash<Key>,
           typename KeyEqual = std::equal_to<Key>,
           typename Allocator = StlAlloc<std::pair<const Key, T> > > 
using hashmap = std::unordered_map<Key, T, Hash, KeyEqual, Allocator>;


// =============================================================================
// Unordered_set <==> hashset
// =============================================================================
template < typename Key,
           typename Hash = std::hash<Key>,
           typename KeyEqual = std::equal_to<Key>,
           typename Allocator = StlAlloc<Key> >
using hashset = std::unordered_set<Key, Hash, KeyEqual, Allocator>;




template <typename ... Ts>
struct activate
{
  tp::activate * tpc;
  activate(activate const &) = delete;
  void operator=(activate const &) = delete;
  activate (int && maxCore = 1, Ts ...) { tpc = new tp::activate (std::move(maxCore)); }
  ~activate () { delete tpc; }
};




template <typename ... Ts>
constexpr void linearize ( Ts & ... objs ) {};


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













