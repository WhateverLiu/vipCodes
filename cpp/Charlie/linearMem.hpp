#pragma once
#include <cstdlib>
#include <algorithm>
#include <stdexcept>
#include <atomic>
#include <iostream>
#include <mutex>
#include <memory>
#include "parFor.hpp"


namespace LinearMem {


struct Bucket
{
  std::size_t *begin, *end, *cap, *maxEnd; // maxEnd is used to how far the end pointer can maximally go.
  void reserve(std::size_t N)
  {
    N = std::max<std::size_t>(3, N);
    begin = new std::size_t [N];
    if ( (std::size_t(begin) & std::size_t(15)) != 0 ) [[unlikely]] 
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




std::size_t bucketMinSize_t = 0, workerBucketMinSize_t = 0;
struct Arena
{
  std::size_t maxSize_tAlloced;
  std::vector<Bucket> * Vptr; // Typically points to Vprimary
  std::vector<Bucket> Vprimary;
  std::vector<Bucket> Vsecondary; // Rarely used for linearization.
  Arena() { Vptr = nullptr; }
};
std::vector<Arena> arenas; 


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


inline bool isReleased(std::size_t u)
{
  constexpr const std::size_t dealloced = std::size_t(1) << 63;
  return (u & dealloced) != 0;
}


inline void setRelease(std::size_t & u)
{
  constexpr const std::size_t dealloced = std::size_t(1) << 63;
  u |= dealloced;
}


inline std::size_t extractSize(std::size_t u)
{
  constexpr const std::size_t dealloced_1 = (std::size_t(1) << 63) - 1;
  return u & dealloced_1;
}


void stackRetreat(auto & V)
{
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
  
  
  /**
   * First, have the stack pointer retreat if it can. Secondly, check if the
   * current bucket can hold the new data.
   */
  T * allocate(std::size_t nT) 
  {  
    if (nT == 0) [[unlikely]] return static_cast<T*>(nullptr);
    
    
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
    auto tid = tp::thisThreadIndex();
    auto bucketInitWords = tid == 0 ? bucketMinSize_t : workerBucketMinSize_t;
    
    
    auto & V = *arenas[tid].Vptr;
    stackRetreat(V); // Let the stack pointer retreat if it can.
    
    
    if ( V.back().end + N > V.back().cap ) // Need a new bucket.
    {
      // =====================================================================
      // If there are no variables in stack, and if
      //   the 1st bucket is already too small, just delete it and
      //   allocate a new, larger one.
      // =====================================================================
      if (V.size() == 1 and V.back().end == V.back().begin)
      {
        delete [] V.back().begin;
        V.back().reserve(N);
      }
      else
      {
        // ===================================================================
        // If capacity is reached, do not just emplace_back() because those
        //   extra buckets newly allocated would not have nullptr pointers.
        //   During the destruction of the pool, we loop through 
        //   [0, capacity()) to delete the array. This is why we manually
        //   double the size of V and set null pointers.
        // ===================================================================
        if (V.size() == V.capacity())
        {
          V.resize( V.size() * 2, Bucket() );
          V.resize( V.size() / 2 );
          V.emplace_back( Bucket( std::max(N, bucketInitWords) ) );
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
          if ( nx.begin != nullptr ) // If a bucket already exists.
          {
            V.emplace_back( nx );
            if ( V.back().begin + N > V.back().cap ) // If the existing bucket is too small.
            {
              delete [] V.back().begin;
              V.pop_back();
              V.emplace_back ( Bucket( N ) );
            }
          }
          else V.emplace_back ( Bucket( std::max(N, bucketInitWords) ) );
        } 
      }
      
      
      V.back().end = V.back().begin;
    }
    
    
    rst = V.back().end;
    V.back().end += N;
    V.back().end[-1] = N; 
    arenas[tid].maxSize_tAlloced += N;
    V.back().maxEnd = std::max(V.back().maxEnd, V.back().end);
    
    
    T * Trst = static_cast<T*>(static_cast<void*>(rst));
    return Trst;
  } 
  
  
  /**
   * Just mark the deallocated block. We do not know if the block was allocated
   * by the current thread, so we place stackRetreat() in allocate().
   */
  void deallocate( T * t, std::size_t nT ) noexcept
  { 
    auto x = static_cast<std::size_t*>(static_cast<void*>(t));
    std::size_t N = ((nT * sizeof(T) + 23) >> 4) << 1;
    setRelease( *(x + N - 1) );
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



struct activate
{ 
  bool verbose;
  activate(activate const&) = delete;
  void operator=(activate const&) = delete;
  
  
  tp::activate * tphandle;
  
  
  /**
   * Activate the memory pool.
   * 
   * @tparam bucketSizeUnit  A character in `{'B', 'K', 'M', 'G'}`, i.e. 
   * {byte, KB, MB, GB}.
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
  activate(
    bool verbose = false,
    int && maxCore = 1, 
    std::size_t bucketSize = 64, // MB
    std::size_t workerBucketSize = 13, // MB
    char bucketSizeUnit = 'M'
  )
  {
    this->verbose = verbose;
    if (!(bucketSizeUnit == 'B' or bucketSizeUnit == 'K' or 
          bucketSizeUnit == 'M' or bucketSizeUnit == 'G'))
      throw std::runtime_error(
          "Bucket size unit is not one of "
          "{'B', 'K', 'M', 'G'}, i.e. "
          "byte, KB, MB, GB");
    
    
    // bucketMinSize_t: space measured in number of words.
    if (bucketSizeUnit == 'B') 
    {
      bucketMinSize_t = bucketSize;
      workerBucketMinSize_t = workerBucketSize;
    } 
    else if (bucketSizeUnit == 'K')
    {
      bucketMinSize_t = bucketSize * 1000;
      workerBucketMinSize_t = workerBucketSize * 1000;
    }
    else if (bucketSizeUnit == 'M')
    {
      bucketMinSize_t = bucketSize * 1000 * 1000;
      workerBucketMinSize_t = workerBucketSize * 1000 * 1000;
    }
    else
    {
      bucketMinSize_t = bucketSize * 1000 * 1000 * 1000;
      workerBucketMinSize_t = workerBucketSize * 1000 * 1000 * 1000;
    }
    bucketMinSize_t = std::max<std::size_t>(1, bucketMinSize_t / 8);
    workerBucketMinSize_t = std::max<std::size_t>(1, workerBucketMinSize_t / 8);
    
    
    tphandle = new tp::activate(std::move(maxCore));
    
    
    // =========================================================================
    // If thread pool is not activated, use empty lock.
    // =========================================================================
    arenas.resize(maxCore);
    for (int a = 0; a < maxCore; ++a)
    {
      arenas[a].Vptr = &arenas[a].Vprimary;
      auto & V = *arenas[a].Vptr;
      V.resize(5, Bucket()); // Do not use .reserve(). Need all the buckets have nullptr initializations.
      V.resize(0);
      
      
      // =======================================================================
      // For the main thread, preallocate memory of size `bucketMinSize_t`.
      //   For the worker threads, preallocate memory of size 
      //   `bucketMinSize_t / (maxCore - 1)`
      // =======================================================================
      auto bsize = a == 0 ? bucketMinSize_t : workerBucketMinSize_t;
      V.emplace_back( Bucket( bsize ) );
      arenas[a].maxSize_tAlloced = 0;
    }
    
    
    // Vptr = &Vprimary;
    // auto & V = *Vptr;
    // V.resize(5, Bucket()); // Do not use .reserve(). Need all the buckets have nullptr initializations.
    // V.resize(0);
    // V.emplace_back( Bucket( bucketMinSize_t ) );
    // maxSize_tAlloced = 0;
  } 
  
  
  
  ~activate()
  {
    int mxc = tp::maxCore();
    delete tphandle;
    std::size_t peakMemUsage = 0, maxSize_tAlloced = 0;
    std::vector<int> NbucketUsed;  
    if (verbose) NbucketUsed.reserve(mxc);
    for (int a = 0, aend = mxc; a < aend; ++a)
    {
      auto & V = *arenas[a].Vptr;
      std::size_t i = 0;
      for (std::size_t iend = V.capacity(); 
           i < iend and V[i].begin != nullptr; ++i)
      {
        peakMemUsage += V[i].maxEnd - V[i].begin;
        delete [] V[i].begin; 
      }
      maxSize_tAlloced += arenas[a].maxSize_tAlloced;
      NbucketUsed.emplace_back(i);
    }
    
    
    if (verbose)
    {
      std::cout << "Peak memory usage = " << 
        peakMemUsage * 8.0 / (1000 * 1000) << " MB.\n";
      std::cout << "Total size of all objects allocated throughout the run = " << 
        maxSize_tAlloced * 8.0 / (1000 * 1000) << " MB.\n";
      for (int i = 0, iend = NbucketUsed.size(); i < iend; ++i)
        std::cout << "Thread " << i << " used " << NbucketUsed[i] << " buckets.\n";
    }
    
    
    decltype(arenas)().swap(arenas);
  }
};




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
 * The function works even if the objects are on another thread's memory space.
 * 
 * The only requirements for `objs` are that they have empty constructors.
 *
 */
template <typename ... Ts>
void linearize ( Ts & ... objs )
{
  if (arenas.size() == 0) return;
  if ( sizeof...(objs) == 0 ) [[unlikely]] return;
  auto tid = tp::thisThreadIndex();
  auto & Vptr = arenas[tid].Vptr;
  auto & Vprimary = arenas[tid].Vprimary;
  auto & Vsecondary = arenas[tid].Vsecondary;
  if (Vptr == nullptr or Vptr->size() == 0) return;
  
  
  // ===========================================================================
  // Change to the secondary stack which the allocator will be directed to.
  // ===========================================================================
  Vsecondary.resize(Vprimary.size(), Bucket()); // Do not use .reserve. This is to ensure null pointers are properly initialized.
  Vsecondary.resize(0);
  Vsecondary.emplace_back( Bucket( 
      tid == 0 ? bucketMinSize_t : workerBucketMinSize_t ) );
  
  
  // ===========================================================================
  // First copy the object to the secondary space. Then destruct the object
  //   in the primary space. Finally copy the object in the secondary space
  //   to the primary space.
  // ===========================================================================
  ([&] { 
    Vptr = &Vsecondary;
    using objType = std::remove_reference<decltype(objs)>::type;
    objType objCopy = objs;
    Vptr = &Vprimary;
    objs.~objType(); // Deallocate all heap memory.
    // =========================================================================
    // Zero all the bits of objs on the stack to avoid error from 
    //   the 2nd time of calling the destructor when `objs` goes out of scope.
    // =========================================================================
    std::memset((void*)&objs, 0, sizeof(objs)); 
    objs = objCopy;
  } (), ...);
  
  
  for (auto & x: Vsecondary) delete [] x.begin; 
  std::remove_reference<decltype(Vsecondary)>::type().swap(Vsecondary);
}




}




