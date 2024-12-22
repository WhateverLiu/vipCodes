#include "ThreadPool.hpp"


namespace tp {


Charlie::ThreadPool tpool;


struct activate
{
  activate(activate const &) = delete;
  void operator=(activate const &) = delete;
  
  
  activate(int && maxCore = std::thread::hardware_concurrency()) {  
    tpool.reset( std::move(maxCore) ); }
  ~activate() { tpool.destroy(); }
};


/**
 * It has been tested. Given a vector of 100 sorted numbers, binary search
 * an element takes about the same time as double precision multiplication.
 * Binary search takes about 2.5x time of double precision multiplication
 * when the number of elements goes up to 1000.
 * 
 * Do not try to optimize the search of thread index here. Hashing and more
 * complex structure will probably only increase the overhead.
 */
std::size_t thisThreadIndex()
{
  if ( tpool.maxCore == 1 ) return 0;
  auto tid = std::this_thread::get_id();
  return tid == tpool.mainThreadID ? 0 : std::lower_bound(
    tpool.workerThreadIDs, 
    tpool.workerThreadIDs + tpool.maxCore, 
    tid) - tpool.workerThreadIDs + 1;
} 


void parFor(std::size_t begin, std::size_t end,
            std::function <bool(std::size_t, std::size_t)> && run,
       std::size_t grainSize = 1,
       std::function <bool(std::size_t)> && beforeRun = 
         [](std::size_t) { return false; },
       std::function <bool(std::size_t)> && afterRun  = 
         [](std::size_t) { return false; })
{
  tpool.parFor( begin, end, std::move(run), grainSize, 
                std::move(beforeRun), std::move(afterRun) );
}


int maxCore() { return tpool.maxCore; }


}



