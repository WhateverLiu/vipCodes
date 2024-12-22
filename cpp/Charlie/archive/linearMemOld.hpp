#pragma once
#include <cstdlib>
#include <algorithm>
#include <stdexcept>
#include <atomic>
#include <iostream>
#include <mutex>
#include <memory>
#include "ThreadPool2.hpp"


namespace LinearMem {


struct Bucket
{
  std::size_t *begin, *end, *cap, *maxEnd; // maxEnd is used to how far the end pointer can maximally go.
  void reserve(std::size_t N)
  {
    N = std::max<std::size_t>(3, N);
    begin = new std::size_t [N];
    if ( (std::size_t(begin) & std::size_t(15)) != 0 ) 
      throw std::runtime_error(
          "Operator new did not allocate 16-byte aligned memory ?!");
    maxEnd = end = begin;
    cap = begin + N;
    // =========================================================================
    // Interestingly, uninitialized memory will not show up on Linux htop.
    //   Only when the memory is written will it be shown.
    // std::fill(begin, cap, std::size_t(0-1) );
    // =========================================================================
  }
  Bucket(std::size_t N) { reserve(N); }
  Bucket () { begin = end = cap = maxEnd = nullptr; }
  
  
  // ===========================================================================
  // ~Bucket () { delete [] begin; begin = end = cap = nullptr; }
  //   Omit destructor so that std::vector::pop_back() will not deallocate
  //   the memory.
  // ===========================================================================
};


std::vector<Bucket> Vprimary;
std::vector<Bucket> Vsecondary;
std::vector<Bucket> * Vptr;


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
constexpr const bool useMutex = false;
std::atomic_flag af;
std::mutex mx;
std::size_t bucketMinSize_t;
std::size_t maxSize_tAlloced;


void (*lockPtr)();
void lockNull(){}
void lock()
{
  if constexpr (useMutex) mx.lock();
  else while (std::atomic_flag_test_and_set_explicit(
      &af, std::memory_order_acquire));
}


void (*unlockPtr)();
void unlockNull(){}
void unlock()
{
  if constexpr (useMutex) mx.unlock();
  else std::atomic_flag_clear_explicit(&af, std::memory_order_release);
}


bool isReleased(std::size_t u)
{
  constexpr const std::size_t dealloced = std::size_t(1) << 63;
  return (u & dealloced) != 0;
}


void setRelease(std::size_t & u)
{
  constexpr const std::size_t dealloced = std::size_t(1) << 63;
  u |= dealloced;
}


std::size_t extractSize(std::size_t u)
{
  constexpr const std::size_t dealloced_1 = (std::size_t(1) << 63) - 1;
  return u & dealloced_1;
}


void stackRetreat()
{
  auto & V = *Vptr;
  while (true)
  { 
    if (V.back().end == V.back().begin)
    { 
      if (V.size() == 1) break;
      V.pop_back();
    } 
    else
    { 
      if ( !isReleased( V.back().end[-1] ) ) break;
      V.back().end -= extractSize( V.back().end[-1] );
    }
  }
}




template <typename T>
struct Pool
{
  typedef T value_type;
  
  
  Pool() = default;
  
  
  template <typename U> 
  constexpr Pool ( const Pool <U> & ) noexcept {}
  
  
  T * allocate(std::size_t nT) 
  {  
    if (nT == 0) [[unlikely]] return static_cast<T*>(nullptr);
    // std::cout << "allocate!\n";
    
    // =========================================================================
    // std::size_t N = (nT * sizeof(T) + 8 + 15) / 16 * 2: 
    //
    //   (i) nT * sizeof(T) is the number of bytes of content.
    //
    //   (ii) nT * sizeof(T) + 8 is the number of bytes of {content + size info}.
    //
    //   (iii) (nT * sizeof(T) + 8 + 15) / 16 is the number of 16-byte blocks
    //     (2 x std::size_t) that will contain {content + size info}.
    //
    //   (iv) (nT * sizeof(T) + 8 + 15) / 16 * 2 is the number of 
    //     std::size_t for the above container.
    // =========================================================================
    std::size_t N = ((nT * sizeof(T) + 23) >> 4) << 1;
    
    
    std::size_t* rst = nullptr;
    lockPtr();
    {
      auto & V = *Vptr;
      if (V.back().end + N >= V.back().cap) // Need another bucket.
      {
        // =====================================================================
        // If there are no variables in stack, and if
        //   the 1st bucket is already too small, just delete it and
        //   allocate a new, larger one.
        // =====================================================================
        if (V.size() == 1 and V.back().end == V.back().begin)
        {
          delete [] V[0].begin;
          V[0].reserve(N);
        }
        else
        {
          // ===================================================================
          // If capacity is reached, do not just emplace_back() because those
          //   extra buckets newly allocated would not have nullptr pointers.
          // ===================================================================
          if (V.size() == V.capacity())
          {
            V.resize( V.size() * 2, Bucket() );
            V.resize( V.size() / 2 );
            V.emplace_back( Bucket( std::max(N, bucketMinSize_t) ) );
          }
          // ===================================================================
          // If capacity is not reached, check if the next bucket already had
          //   been assigned with new[]. If yes, emplace_back() exactly the
          //   same bucket. Otherwise emplace_back() a newly initialized 
          //   bucket.
          // ===================================================================
          else
          {
            auto & nx = *(V.data() + V.size());
            if ( nx.begin != nullptr ) V.emplace_back( nx );
            else V.emplace_back ( Bucket( std::max(N, bucketMinSize_t) ) );
          } 
        }
        
        
        V.back().end = V.back().begin;
      }
      
      
      rst = V.back().end;
      V.back().end += N;
      V.back().end[-1] = N; 
      maxSize_tAlloced += N;
      V.back().maxEnd = std::max(V.back().maxEnd, V.back().end);
    }
    unlockPtr();
    
    
    T * Trst = static_cast<T*>(static_cast<void*>(rst));
    return Trst;
  } 
  
  
  void deallocate( T * t, std::size_t nT ) noexcept
  { 
    auto* x = static_cast<std::size_t*>(static_cast<void*>(t));
    std::size_t N = ((nT * sizeof(T) + 23) >> 4) << 1;
    
    
    lockPtr();
    {
      auto & V = *Vptr;
      if ( V.back().end != x + N ) 
        // =====================================================================
        // If the block to be deallocated is not the last block in stack.
        // =====================================================================
      {
        setRelease( *(x + N - 1) );
      }
      else
      {    
        V.back().end = x; // Stack pointer retreats.
        stackRetreat();
      }
    }
    unlockPtr();
  }
  
  
  /**
   * Here the new operator are placement new. It will not call `malloc` or `new`
   * if U is default constructible.
   */
  template <typename U, typename... Args>
  void construct(U* ptr, Args&&... args)
  {
    if constexpr (std::is_nothrow_default_constructible<U>::value and 
                    sizeof...(args) == 0)
      ::new(static_cast<void*>(ptr)) U;
    else 
      ::new(static_cast<void*>(ptr)) U(std::forward<Args>(args)...);
  }
};


template <typename S, typename U>
constexpr bool operator == (const Pool<S> &t, const Pool<U> &u) { return true; }


template <typename S, typename U>
constexpr bool operator != (const Pool<S> &t, const Pool<U> &u) { return false; }



template <char bucketSizeUnit = 'M', bool verbose = true>
struct activate
{ 
  activate(activate const&) = delete;
  void operator=(activate const&) = delete;
  
  
  tp::activate *tphandle;
  
  
  /**
   * Activate the memory pool.
   * 
   * @tparam bucketSizeUnit  A character in `{'B', 'K', 'M', 'G'}`, i.e. 
   * {byte, KiB (1024 bytes), MiB (1024 KiBs), GiB (1024 MiBs)}.
   * 
   * @tparam verbose  True if the memory allocator should print usage
   * information before its destruction.
   * 
   * @param maxCore  Number of threads to be invoked. `maxCore > 1`
   * will trigger the construction of the thread pool, and activate the
   * mutex guard for allocation and deallocation.
   * 
   * @param bucketSize  How many `bucketSizeUnit`s should be allocated for
   * each bucket.
   */
  activate(int && maxCore = 1, 
           std::size_t bucketSize = 1)
  {
    static_assert(bucketSizeUnit == 'B' or bucketSizeUnit == 'K' or 
                    bucketSizeUnit == 'M' or bucketSizeUnit == 'G', 
                      "Bucket size unit is not one of "
                      "{'B', 'K', 'M', 'G'}, i.e. "
                      "byte, KiB, MiB, GiB");
    
    
    if constexpr (bucketSizeUnit == 'B') 
      bucketMinSize_t = bucketSize / 8;
    else if constexpr (bucketSizeUnit == 'K')
      bucketMinSize_t = bucketSize * 1024 / 8;
    else if constexpr (bucketSizeUnit == 'M')
      bucketMinSize_t = bucketSize * 1024 / 8 * 1024;
    else
      bucketMinSize_t = bucketSize * 1024 / 8 * 1024 * 1024;
    
    
    bucketMinSize_t = std::max<std::size_t>(1, bucketMinSize_t);
    
    
    tphandle = new tp::activate(std::move(maxCore));
    
    
    // =========================================================================
    // If thread pool is not activated, use empty lock.
    // =========================================================================
    if (maxCore > 1)
    {
      lockPtr = &lock;
      unlockPtr = &unlock;
    }
    else
    {
      lockPtr = &lockNull;
      unlockPtr = &unlockNull;
    }
    
    
    Vptr = &Vprimary;
    auto & V = *Vptr;
    V.resize(5, Bucket()); // Do not use .reserve(). Need all the buckets have nullptr initializations.
    V.resize(0);
    V.emplace_back( Bucket( bucketMinSize_t ) );
    af.clear();
    maxSize_tAlloced = 0;
  } 
  
  
  ~activate()
  { 
    delete tphandle;
    auto &V = *Vptr;
    std::size_t i = 0;
    std::size_t peakMemUsage = 0;
    for (std::size_t iend = V.capacity(); 
         i < iend and V[i].begin != nullptr; ++i)
    {
      peakMemUsage += V[i].maxEnd - V[i].begin;
      delete [] V[i].begin; 
    }
    
    
    if constexpr (verbose)
    {
      std::cout << "Peak memory usage = " << 
        peakMemUsage * 8.0 / (1024 * 1024) << " MiB.\n";
      std::cout << "Number of buckets used = " << i << "\n";
      std::cout << "Total size of all objects allocated throughout the run = " << 
        maxSize_tAlloced * 8.0 / (1024 * 1024) << " MiB.\n";
    }
    
    
    decltype(Vprimary)().swap(Vprimary);
    decltype(Vsecondary)().swap(Vsecondary);
    Vptr = nullptr;
    lockPtr = nullptr;
    unlockPtr = nullptr;
    maxSize_tAlloced = 0;
    bucketMinSize_t = 0 - 1;
  }
};




template <int i, int size>
void assign2tuples(auto & x, auto & y)
{
  if constexpr (i < size)
  {
    std::get<i>(x) = std::get<i> (y);
    assign2tuples <i + 1, size> (x, y);   
  }
}


template <int i, int size>
void moveYintoX(auto & x, auto & y)
{
  if constexpr (i < size)
  {
    std::get<i>(x) = std::move(std::get<i> (y));
    moveYintoX <i + 1, size> (x, y);   
  } 
} 



/**
 * Relocate objects of interest to linearize their memory layout.
 * 
 * The steps are as follows:
 * 
 *   -# Copy the objects to the secondary stack memory 
 *   space, the copied objects will have a compact, linear memory layout. 
 *   
 *   -# Remove the objects in the primary stack space. This is done by using 
 *   `std::move` to move the resources into temporary objects, which will 
 *   be auto destructed when going out of scope. 
 *   
 *   -# Copy resources from the secondary stack to the primary stack. 
 *   
 * Between the memory blocks that belong to any two objects in `...objs`, 
 * if there exists a block held by an in-use alien object, the original 
 * space allocated for `...objs` cannot be released. This is 
 * due to the memory's linear nature. Otherwise the original 
 * memory will be reused to store the linearized objects.
 * 
 * The initial motivation behind this function is to linearize the 
 * entire memory stack and remove "holes". This is done by
 * supplying the function all the objects that need to be saved.
 * 
 * All pointers and iterators associated with the original objects
 * are invalidated.
 *
 */
template <typename ... Ts>
void linearize ( Ts & ... objs )
{
  if ( sizeof...(objs) == 0 ) return;
  if (Vptr == nullptr or Vptr->size() == 0) return; // If the stack is not
  //   activated, return.
  
  
  lockPtr();
  {
    // =========================================================================
    // Change to the secondary stack which the allocator will be directed to.
    // =========================================================================
    Vptr = &Vsecondary;
    Vptr->resize(Vprimary.size(), Bucket()); // Do not use .reserve. 
    Vptr->resize(0);
    Vptr->emplace_back( Bucket( bucketMinSize_t ) );
    
    
    // std::cout << "1.1\n";
    // =========================================================================
    // Copy the objects of interest using the new allocator.
    // =========================================================================
    { // This scope is necessary for properly destructing `copied`.
      
      
      std::tuple<Ts...> copied(objs...);
      // std::cout << "1.2\n";
      Vptr = &Vprimary; // Change it back to the original stack.
      // std::cout << "After copying, Vsecondary.back().end - Vsecondary.back().begin = " <<
      //   Vsecondary.back().end - Vsecondary.back().begin << "\n";
      
      
      // =========================================================================
      // Move the original resources into temporary objects which are then
      //   auto destructed going out of scope.
      // =========================================================================
      
      
      // std::cout << "Before move objs to temp objects: ";
      // std::cout << Vprimary.back().end - Vprimary.back().begin << "\n";
      ([&] { 
        using objType = std::remove_reference<decltype(objs)>::type;
        if constexpr ( std::is_destructible<objType>::value )
          objs.~objType();  
      } (), ...);
      // std::cout << "1.3\n";
      // std::cout << "After move objs to temp objects, Vprimary ptr = ";
      // std::cout << Vprimary.back().end - Vprimary.back().begin << "\n";
      
      
      // std::cout << "Before assign2tuples, Vprimary ptr = ";
      // std::cout << Vprimary.back().end - Vprimary.back().begin << "\n";
      // =========================================================================
      // Copy the objects in the secondary stack to the original.
      // =========================================================================
      auto original = std::forward_as_tuple(objs...);
      
      
      // std::cout << "Before copying copied, endPtr = " << Vprimary.back().end - Vprimary.back().begin << "\n";
      auto copiedCopy = copied;
      // std::cout << "After copying copied, endPtr = " << Vprimary.back().end - Vprimary.back().begin << "\n";
      
      
      // std::cout << "assign2tuples() begins======\n";
      // assign2tuples < 0, sizeof...(objs) > (original, copied);
      // std::cout << "Before moveYintoX, endPtr = " << Vprimary.back().end - Vprimary.back().begin << "\n";
      moveYintoX < 0, sizeof...(objs) > (original, copiedCopy);
      // std::cout << "After moveYintoX, endPtr = " << Vprimary.back().end - Vprimary.back().begin << "\n";
      // std::cout << "assign2tuples() ends======\n";
      // std::cout << "is lvalue reference = " << 
      //   std::is_lvalue_reference< decltype(std::get<0>(original)) >::value << "\n";
      // std::cout << "&std::get<0>(original) = " << &std::get<0>(original) << "\n";
      
      
      // std::cout << "sizeof...(objs) = " << sizeof...(objs) << "\n";
      // std::cout << "After assign2tuples: Vprimary ptr = ";
      // std::cout << Vprimary.back().end - Vprimary.back().begin << ", ";
      // std::cout << "Vsecondary ptr = ";
      // std::cout << Vsecondary.back().end - Vsecondary.back().begin << "\n";
      
      
      // =======================================================================
      // Remove the secondary space.
      // =======================================================================
      Vptr = &Vsecondary;
      // copied gets destructed.
    }
    // using copiedType = decltype(copied);
    // copied.~copiedType();
    Vptr = &Vprimary;
    
    
    for (auto & x: Vsecondary) { delete [] x.begin; x.begin = nullptr; }
    Vsecondary.resize(0);
    // std::cout << "1.5\n";
    
  }
  unlockPtr();
}




}




