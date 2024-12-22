

namespace Charlie {


/**
 * Given a nonnegative integer, return the lowest power of two that is no less.
 * 
 * From <https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2>
 * 
 */
template <typename T>
uint64_t np2 ( T v )
{
  if constexpr ( std::is_same<float, T>::value )
    return np2 ( uint32_t(std::round(v)) );
  else if constexpr ( std::is_same<double, T>::value )
    return np2 ( uint64_t(std::round(v)) );
  else 
  {
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    if constexpr (sizeof(v) == 1) { ++v; return v; }
    else
    {
      v |= v >> 8;
      if constexpr (sizeof(v) == 2) { ++v; return v; }
      else
      {
        v |= v >> 16;
        if constexpr (sizeof(v) == 4) { ++v; return v; }
        else
        {
          v |= v >> 32;
          ++v;
          return v;
        }
      }
    }
  }
}



}


// def f(v):
//   v -= 1;
//   v |= v >> 1;
//   v |= v >> 2;
//   v |= v >> 4;
//   v |= v >> 8;
//   v |= v >> 16;
//   v |= v >> 32;
//   v += 1;
//   return v















