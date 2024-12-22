#pragma once
#include <cstdlib>
#include <algorithm>
#include <stdexcept>
#include <atomic>
#include <iostream>
#include <mutex>
#include "ThreadPool2.hpp"





/**
 * Usage pattern:
 * 
 * template <typename T> using vec = std::vector<T, vp::VecPool<T> >;
 */
namespace vp {


std::size_t getNbyte(void * x)
{
  return *((static_cast<std::size_t*> (x)) - 1);
}


void * getSourcePtr(void * x)
{
  return ( static_cast<std::size_t*> (x) ) - 2;
}


// =============================================================================
// It has been tested. Using spin lock is actually slower than using mutex.
//   By running random allocations, mutex gives
//   user.self   sys.self    elapsed user.child  sys.child 
//   8.164033   1.137467   0.825000   0.000000   0.000000
// while spin lock gives:
//   user.self   sys.self    elapsed user.child  sys.child 
//   12.6423000  0.3629333  0.8507333  0.0000000  0.0000000
// Even in single threaded environment, mutex seems advantagerous.
// =============================================================================
bool isMultithreaded = true;
constexpr const bool useMutex = true;
std::atomic_flag lock;
std::mutex mx;
unsigned Nidle = 0, cntSize = 0;
void ** idle = nullptr;
unsigned Nreused, Nrenewed;




template <bool verbose = true>
struct activate
{
  activate(activate const&) = delete;
  void operator=(activate const&) = delete;
  
  
  tp::activate *tphandle;
  
  
  /**
   * Activate the vector pool.
   * 
   * @param maxCore  The number of threads to be invoked. `maxCore > 1`
   * will trigger the construction of the thread pool, and activate the
   * mutex guard of allocation and deallocation.
   * 
   * @param n  The initial number of buffer slots. 
   */
  activate(int && maxCore = 1, unsigned n = 30)
  {
    isMultithreaded = maxCore > 1;
    tphandle = new tp::activate(std::move(maxCore));
    n = std::max<unsigned>(1, n);
    idle = static_cast<void**>(std::malloc(sizeof(void*) * n));
    cntSize = n;
    Nidle = 0;
    Nrenewed = 0;
    Nreused = 0;
    lock.clear();
  }
  
  
  ~activate()
  {
    delete tphandle;
    std::size_t totalByte = 0;
    for (unsigned i = 0; i < Nidle; ++i) 
    {
      if constexpr (verbose) totalByte += getNbyte(idle[i]);
      std::free( getSourcePtr(idle[i]) );
    }
    std::free (idle);
    idle = nullptr;
    if constexpr (verbose)
    {
      std::cout << "VecPool peak space consumed = " << 
        totalByte / (1024.0 * 1024.0) << " MiB.\n";
      std::cout << "VecPool peak number of buffers in use = " << Nidle << ".\n";
      std::cout << "VecPool buffer reallocated " << Nrenewed << " times.\n";
      std::cout << "VecPool buffer reused " << Nreused << " times.\n";
      std::cout << "VecPool recycle efficiency = " << 
        double(Nreused) / (Nrenewed + Nreused) << ".\n";
    }
    cntSize = Nidle = 0;
    Nreused = Nrenewed = 0;
  }
};




template <typename T>
struct VecPool
{
  typedef T value_type;
  
  
  VecPool()
  {
    if (idle == nullptr)
      throw std::runtime_error("Memory pool uninitialized.");
  }
  
  
  void * newAlloc (std::size_t Nbyte)
  {
    // =========================================================================
    // malloc defaults to 16-byte alignment on x64. So pad 2 x size_t.
    // =========================================================================
    auto p = static_cast<std::size_t*> (std::malloc(
      Nbyte + sizeof(void*) * 2));
    
    
    // =========================================================================
    // Curious check: does std::malloc() always allocate 16 byte aligned
    //   memory? Yes. And no for 32, 64 bytes...
    // =========================================================================
    // if ( (std::size_t(p) & std::size_t(32 - 1)) != 0 )
    //   std::cout << "malloc did not allocate 32 byte aligned memory!\n";
    // =========================================================================
    
    
    p[1] = Nbyte;
    return static_cast<void*>(p + 2);
  }
  
  
  T * allocate(std::size_t n) 
  {
    auto Nbyte = sizeof(T) * n;
    void *rst = nullptr;
    
    
    // =========================================================================
    // Taken from https://en.cppreference.com/w/cpp/atomic/atomic_flag_test_and_set
    //   An implementation of a spin lock.
    // =========================================================================
    if (isMultithreaded)
    {
      if constexpr (useMutex) mx.lock();
      else while (std::atomic_flag_test_and_set_explicit(&lock, std::memory_order_acquire));
    }
    
    
    if (Nidle == 0) rst = newAlloc(Nbyte);
    else
    {
      auto I = idle[Nidle - 1];
      if (getNbyte(I) >= Nbyte) { rst = I; Nreused += 1; }
      else
      {
        Nrenewed += 1;
        std::free(getSourcePtr(I));
        rst = newAlloc(Nbyte);
      }
      Nidle -= 1;  
    }
    
    
    if (isMultithreaded)
    {
      if constexpr (useMutex) mx.unlock();
      else std::atomic_flag_clear_explicit(&lock, std::memory_order_release);
    }
    
    
    return static_cast<T*> (rst);
  } 
  
  
  void deallocate(T * p, std::size_t n) noexcept
  {
    if (isMultithreaded)
    {
      if constexpr (useMutex) mx.lock();
      else while (std::atomic_flag_test_and_set_explicit(&lock, std::memory_order_acquire));
    }
    
    
    if (Nidle >= cntSize)
    {
      auto ptrNew = static_cast<void**> (std::malloc(sizeof(void*) * cntSize * 2 ));
      std::copy(idle, idle + cntSize, ptrNew);
      std::free(idle);
      cntSize *= 2;
      idle = ptrNew;
    } 
    idle[Nidle] = p;
    Nidle += 1;
    
    
    if (isMultithreaded)
    {
      if constexpr (useMutex) mx.unlock();
      else std::atomic_flag_clear_explicit(&lock, std::memory_order_release);
    }
  }
};


template <typename S, typename U>
constexpr bool operator == (const VecPool<S> &t, const VecPool<U> &u) 
{ 
  return true; 
}


// =============================================================================
// Just for testing
// =============================================================================
template <typename T>
struct NaiveAlloc
{
  typedef T value_type;
  T * allocate(std::size_t n)
  {
    std::cout << "Nbyte = " << n * sizeof(T) << "\n";
    return (T*)(malloc(n * sizeof(T)));
  }
  void deallocate(T * p, std::size_t n) noexcept
  {
    free(p);
  }
  
};






}




