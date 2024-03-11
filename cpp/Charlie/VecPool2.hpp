#pragma once
#include "VecPool.hpp"
// #include "isVector.hpp"
// #include "mmcp.hpp"
#include "ThreadPool.hpp"


namespace vp {


std::vector<Charlie::VecPool> CharlieVpool;
void activate()
{
  CharlieVpool.resize(std::thread::hardware_concurrency(), 0);
  CharlieVpool[0].getPool().resize(307);
  CharlieVpool[0].getPool().resize(0);
  for (int i = 1, iend = CharlieVpool.size(); i < iend; ++i)
  {
    CharlieVpool[i].getPool().resize(13);
    CharlieVpool[i].getPool().resize(0);
  }
    
}
void deactivate()
{
  std::vector<Charlie::VecPool>().swap(CharlieVpool);  
}


template <std::size_t Nbyte, typename Xptr, typename Yptr>
inline void swapBuffers(Xptr x, Yptr y)
{
  char t[Nbyte];
  std::memcpy(t, (void*)x, Nbyte);
  std::memcpy((void*)x, (void*)y, Nbyte);
  std::memcpy((void*)y, t, Nbyte);
}


template <typename T>
struct vec: public std::vector<T>
{
  std::size_t whichThread;
  ~vec()
  { 
    std::vector<T> t;
    std::memcpy((void*)&t, (void*)this, sizeof(char*) * 3);
    std::memset((void*)this, 0, sizeof(char*) * 3);
    CharlieVpool[whichThread].recall(t);
  }
  void destruct() // Release the space without returning it to the pool.
  {
    std::vector<T> t;
    std::memcpy((void*)&t, (void*)this, sizeof(char*) * 3);
    std::memset((void*)this, 0, sizeof(char*) * 3);
  }
}; 


// #############################################################################
// Lend a nested vector. The innermost type is T, and the sizes of the nests
//   are given by Args...
// #############################################################################
template <typename T, typename... Args>
inline auto lend(std::size_t whichThread, std::size_t size, Args... restSizes) // restSizes is a pack.
{
  auto t = CharlieVpool[whichThread].lend<T>(size, restSizes...);
  vec<  typename decltype(t)::value_type   > rst;
  rst.whichThread = whichThread;
  swapBuffers<sizeof(char*) * 3>(&rst, &t);
  return rst;
}


template <typename T>
inline void recall(vec<T> &v) { v.~vec(); }




}










