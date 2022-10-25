#include "charlieThreadPool.hpp"


#ifndef vec 
#define vec std::vector
#endif


// Use Multithreaded merge sort if the vector is big enough and a pointer to
// an existing CharlieThreadPool is given.


// Can only sort elements in a block of memory.
class CharlieSort
{
  vec<uint64_t> offset;
  vec<uint64_t> v;
  
  
public:
  template <typename num, typename Compare>  
  void operator() (num *x, num *xend, Compare f, 
                CharlieThreadPool *cp = nullptr)
  {
    if (cp == nullptr or xend - x < 3000 or cp->maxCore <= 1) 
    { 
      std::sort(x, xend, f); return; 
    }
    
    
    uint64_t size = xend - x;
    uint64_t vsize = (size * sizeof(num) + sizeof(uint64_t) - 1) / 
      sizeof(uint64_t);
    v.resize(vsize);
    
    
    uint64_t Nblock = std::round(std::exp2(uint64_t(std::round(
      std::log2(cp->maxCore * 4.0) / 2)) * 2));
    
    
    offset.resize(Nblock + 1);
    offset[0] = 0;
    double approxBlockSize = (vsize + 0.0) / Nblock;
    for (uint64_t i = 1, iend = offset.size() - 1; i < iend; ++i)
      offset[i] = uint64_t(std::round(approxBlockSize * i));
    offset.back() = vsize;
    cp->parFor(0, Nblock, [&](std::size_t i, std::size_t t)->bool
    {
      std::sort(x + offset[i], x + offset[i + 1], f);
      return false;
    }, 1);
    
    
    num *y = (num*)(&v[0]);
    for (uint64_t delta = 1; delta < Nblock; delta *= 2)
    {
      cp->parFor(0, Nblock / (delta * 2), [&](std::size_t i, std::size_t t)->bool
      {
        std::size_t i2delta = i * 2 * delta;
        uint64_t &begin1 = offset[i2delta];
        uint64_t &end1   = offset[i2delta + delta];
        uint64_t &begin2 = end1;
        uint64_t &end2   = offset[(i + 1) * 2 * delta];
        std::merge(x + begin1, x + end1, x + begin2, x + end2, y + begin1, f);
        return false;
      }, 1);
      std::swap(x, y);
    }
  }
  
  
  template <typename num>
  void operator() (num *x, num *xend,
                CharlieThreadPool *cp = nullptr)
  {
    auto cmp = [](const num &u, const num &v)->bool { return u < v; };
    (*this)(x, xend, cmp, cp);
  }
  
  
  template <typename Iter, typename Compare>  
  void operator() (Iter x, Iter xend, Compare f, 
                CharlieThreadPool *cp = nullptr)
  {
    (*this)(&*x, &*xend, f, cp);
  }
  
  
  template <typename Iter>
  void operator() (Iter x, Iter xend, CharlieThreadPool *cp = nullptr)
  {
    (*this)(&*x, &*xend, cp);
  }
};















































