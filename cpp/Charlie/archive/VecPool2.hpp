#pragma once
#include "VecPool.hpp"
#include <thread>


namespace vp {


std::vector<Charlie::VecPool> CharlieVpool;


struct activate
{
  activate(
    std::size_t &&maxCore = std::thread::hardware_concurrency())
  {
    CharlieVpool.resize(maxCore, 0); 
    CharlieVpool[0].getPool().resize(37);
    CharlieVpool[0].getPool().resize(0);
    for (int i = 1, iend = CharlieVpool.size(); i < iend; ++i)
    { 
      CharlieVpool[i].getPool().resize(13);
      CharlieVpool[i].getPool().resize(0);
    } 
  } 
  ~activate() { std::vector<Charlie::VecPool>().swap(CharlieVpool); }
};


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
  // This is valid! This enables vec to be included as members in other classes
  //   and default to thread 0.
  std::size_t whichThread = 0; 
  
  
  std::vector<T> toStlVec()
  {
    std::vector<T> t;
    std::memcpy((void*)&t, (void*)this, sizeof(char*) * 3);
    std::memset((void*)this, 0, sizeof(char*) * 3);
    return t;
  }
  
  
  void assignTrd(std::size_t whichThread) { this->whichThread = whichThread; }
  
  
  void swap(vec<T> &x) { gswap(x, *this); }
  void swap(std::vector<T> &x) { gswap(x, *this); }
  
  
  // Release the space without returning it to the pool.
  void destruct() { toStlVec(); }
  void toPool() 
  { 
    auto t = toStlVec();
    CharlieVpool[whichThread].recall(t); 
  }
  ~vec() {  toPool(); }
  
}; 


// =============================================================================
// Swap a vp::vec<T> and std::vector<T>
// =============================================================================
template <typename T>
void gswap(vec<T> &x, std::vector<T> &y) 
{
  swapBuffers<sizeof(char*) * 3>(&x, &y); 
}
template <typename T>
void gswap(std::vector<T> &y, vec<T> &x) 
{
  swapBuffers<sizeof(char*) * 3>(&x, &y); 
} 
template <typename T>
void gswap(std::vector<T> &x, std::vector<T> &y) 
{
  x.swap(y);
}  
template <typename T>
void gswap(vec<T> &x, vec<T> &y) 
{
  swapBuffers<sizeof(char*) * 4>(&x, &y); 
}   




// =============================================================================
// Call pattern: vp::lend(0)(3, 5). From thread 0, lend a vector of 3 vectors 
// whose number of elements are equal to 5.
// =============================================================================
template <typename T, typename... Args>
struct lend
{
  std::size_t thrd;
  lend(std::size_t thrd = 0): thrd(thrd) {}
  auto operator()(std::size_t size, Args... restSizes) // restSizes is a pack.
  {
    auto t = CharlieVpool[thrd].lend<T>(size, restSizes...);
    vec<  typename decltype(t)::value_type   > rst;
    rst.whichThread = thrd;
    swapBuffers<sizeof(char*) * 3>(&rst, &t);
    return rst;
  }
};




// =============================================================================
// Explicitly recall an vp::vec
// =============================================================================
template <typename T>
inline void recall(vec<T> &v) { v.~vec(); }


template <typename T>
inline void recall(std::vector<T> &v, std::size_t trd = 0) 
{ 
  CharlieVpool[trd].recall(v); 
}


std::size_t totalAllocByte()
{
  std::size_t rst = 0;
  for (auto &x: CharlieVpool) rst += x.totalAllocByte();
  return rst;
}


std::size_t totalAllocContainers()
{
  std::size_t rst = 0;
  for (auto &x: CharlieVpool) rst += x.totalAllocContainers();
  return rst;
} 



}










