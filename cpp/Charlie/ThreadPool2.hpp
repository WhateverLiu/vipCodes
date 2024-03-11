#include "ThreadPool.hpp"


namespace tp {


Charlie::ThreadPool tpool;
void activate(int &&maxCore) {  tpool.reset( std::move(maxCore) );  }
void deactivate() {  tpool.destroy();  }


void parFor(std::size_t begin, std::size_t end,
            std::function <bool(std::size_t, std::size_t)> &&run,
       std::size_t grainSize = 1,
       std::function <bool(std::size_t)> &&beforeRun = [](std::size_t) { return false; },
       std::function <bool(std::size_t)> &&afterRun  = [](std::size_t) { return false; })
{
  tpool.parFor( begin, end, std::move(run), grainSize, 
                std::move(beforeRun), std::move(afterRun) );
}


int maxCore() { return tpool.maxCore; }


}



