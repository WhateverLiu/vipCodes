#include <string>
#include <string_view>
#include <unordered_map>


template < typename T >
struct MyMem
{
  typedef T value_type;
  
  
  
  
  T * allocate (std::size_t size) 
  { 
    return new T [size];
  } 
  
  void deallocate ( T * t, std::size_t size ) noexcept
  { 
    delete [] t;
  }
}; 


using lstr = std::basic_string<
  char, 
  std::char_traits<char>, 
  MyMem<char>
>; 


struct lstrHash
{ 
  std::size_t operator()(const lstr & x) const
  { 
    std::string_view sv(&x[0], x.size());
    return std::hash<decltype(sv)>()(sv);
  }
}; 


struct lstrEqual
{ 
  bool operator()(const lstr & x, const lstr & y) const
  {
    if (x.size() != y.size()) return false;
    for (auto xi = x.begin(), yi = y.begin(); xi != x.end(); ++xi, ++yi)
    { 
      if ( *xi != *yi ) return false;
    } 
    return true;
  } 
};


template <typename Key, typename T, 
          typename Hash = std::hash<Key>, 
          typename KeyEqual = std::equal_to<Key> > 
using unmap = std::unordered_map <
  Key, T, 
  Hash, 
  KeyEqual, 
  MyMem<std::pair<const Key, T> > 
>;


int main()
{ 
  unmap<lstr, int, lstrHash, lstrEqual> a;
  unmap<lstr, int, lstrHash, lstrEqual> b(10);
} 





