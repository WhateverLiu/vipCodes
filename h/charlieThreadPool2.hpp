#pragma once
#include <thread>
#include <atomic>


struct dynamicTasking
{
  std::size_t jobEnd, grainSize;
  std::atomic<std::size_t> counter;


  // Return false if no job remains.
  bool nextTaskIDrange(std::size_t &begin, std::size_t &end)
  {
    begin = counter.fetch_add(grainSize);
    if (begin >= jobEnd) return false;
    end = std::min(jobEnd, begin + grainSize);
    return true;
  }


  void reset(std::size_t jobBegin, std::size_t jobEnd, std::size_t grainSize)
  {
    counter = jobBegin;
    this->jobEnd = jobEnd;
    this->grainSize = grainSize;
  }


  dynamicTasking(){}
  dynamicTasking(std::size_t jobBegin, std::size_t jobEnd, std::size_t grainSize)
  {
    reset(jobBegin, jobEnd, grainSize);
  }
};


struct CharlieThreadPool
{
  int maxCore;
  volatile bool *haveFood; // haveFood[maxCore] will be appEnd indicator.
  std::thread *tp;
  std::function<bool(std::size_t, std::size_t)> *run;
  std::function<bool(std::size_t)> *beforeRun;
  std::function<bool(std::size_t)> *afterRun;
  dynamicTasking dT; // Will be set by ParaFor object.


  void runJobs(std::size_t threadID) // threadID = 0 is the main thread.
  {
    bool earlyReturn = false;
    earlyReturn = (*beforeRun)(threadID);
    if (earlyReturn) return;
    while (!earlyReturn)
    {
      std::size_t I, Iend;
      if (!dT.nextTaskIDrange(I, Iend)) break;
      for (; I < Iend and !earlyReturn; ++I) earlyReturn = (*run)(I, threadID);
    }
    if (!earlyReturn) (*afterRun)(threadID);
  }


  void live(int threadID)
  {
    while (true)
    {
      // =======================================================================
      // If the thread i has no food, then keep looping.
      // haveFood[maxCore] == true implies the threadpool will be destroyed
      // soon.
      // =======================================================================
      while ( !haveFood[threadID] )
      {
        if ( haveFood[maxCore] ) return; // if (appEnd) return;
      }
      runJobs(threadID);
      haveFood[threadID] = false;
    }
  }


  // ===========================================================================
  // maxCore will be changed if it exceeds the maximum number of cores on machine.
  // ===========================================================================
  void initialize(int &maxCore)
  {
    maxCore = std::max<int> (1, std::min<int> (
      std::thread::hardware_concurrency(), maxCore));
    this->maxCore = maxCore;
    if (maxCore <= 1) return;
    
    
    haveFood = new volatile bool [maxCore + 1]; // haveFood[maxCore] == true
    // would imply the thread pool is about to be destroyed.
    std::fill(haveFood, haveFood + maxCore + 1, false);
    tp = new std::thread [maxCore];
    for (int i = 1; i < maxCore; ++i) // Fire up all the worker threads.
      tp[i] = std::thread(&CharlieThreadPool::live, this, i);
  }


  void destroy()
  {
    if (maxCore <= 1) return;
    haveFood[maxCore] = true;
    for (int i = 1; i < maxCore; ++i) tp[i].join();
    delete [] tp;
    tp = nullptr;
    delete [] haveFood;
    haveFood = nullptr;
  }


  // ===========================================================================
  // maxCore will be changed if it exceeds the maximum number of cores on machine.
  // ===========================================================================
  void reset(int &maxCore)
  {
    maxCore = std::min<int> (
      std::thread::hardware_concurrency(), maxCore);
    if (maxCore != this->maxCore)
    {
      destroy();
      initialize(maxCore);
    }
  }

  // ===========================================================================
  // maxCore will be changed if it exceeds the maximum number of cores on machine.
  // ===========================================================================
  CharlieThreadPool(int &maxCore) { initialize(maxCore); }


  ~CharlieThreadPool() { if (haveFood != nullptr) destroy(); }


  // If `run` or `beforeRun` or `afterRun` are lvalues, use std::move().
  void parFor(std::size_t begin, std::size_t end,
              std::function<bool(std::size_t, std::size_t)> &&run,
              std::size_t grainSize,
              std::function<bool(std::size_t)> &&beforeRun = [](std::size_t) { return false; },
              std::function<bool(std::size_t)> &&afterRun  = [](std::size_t) { return false; })
  {
    if (maxCore <= 1)
    {
      for (; begin < end; ++begin) run(begin, 0);
      return;
    }
    this->run = &run;
    this->beforeRun = &beforeRun;
    this->afterRun = &afterRun;
    this->dT.reset(begin, end, grainSize);
    std::fill(this->haveFood, this->haveFood + this->maxCore, true); // Kick off job runs.
    this->runJobs(0); // Main thread also runs jobs.
    bool allfinished = false;
    while (!allfinished)
    {
      allfinished = true;
      for (int i = 1; i < this->maxCore; ++i) 
        allfinished &= !this->haveFood[i];
    }
  }
};



// =============================================================================
// DO NOT DELETE!!!
// Example use:
// =============================================================================
// // [[Rcpp::export]]
// int paraSummation(IntegerVector x, int maxCore = 15, int grainSize = 100)
// {
//   IntegerVector S(maxCore);
//   CharlieThreadPool ctp(maxCore);
//   ctp.parFor(0, x.size(), [&](std::size_t i, std::size_t t)->bool
//   {
//     S[t] += (x[i] % 31 + x[i] % 131 + x[i] % 73 + x[i] % 37 + x[i] % 41) % 7;
//     return false; // Return true indicates early return.
//   }, grainSize,
//   [](std::size_t t)->bool{ return false; },
//   [](std::size_t t)->bool{ return false; });
//   return std::accumulate(S.begin(), S.end(), 0);
// }


// =============================================================================
// # R code to test:
// Rcpp::sourceCpp('tests/testThreadPool.cpp', verbose = T)
//   tmp2 = sample(1000L, 3e7, replace= T) 
//   system.time({cppRst = paraSummation(tmp2, maxCore = 100, grainSize = 100)})
//   system.time({truth = sum((tmp2 %% 31L + tmp2 %% 131L + tmp2 %% 73L + 
//     tmp2 %% 37L + tmp2 %% 41L) %% 7L)})
//   cppRst - truth
// =============================================================================








// // [[Rcpp::export]]
// double paraSummationFloat(NumericVector x, int maxCore = 15, int grainSize = 100)
// {
//   NumericVector S(maxCore);
//   CharlieThreadPool ctp(maxCore);
//   ctp.parFor(0, x.size(), [&](std::size_t i, std::size_t t)->bool
//   {
//     S[t] += (x[i] / 31 + x[i] / 131 + x[i] / 73 + x[i] / 37 + x[i] / 41) / 7;
//     return false; // Return true indicates early return.
//   }, grainSize,  
//   [](std::size_t t)->bool{ return false; }, 
//   [](std::size_t t)->bool{ return false; });
//   return std::accumulate(S.begin(), S.end(), 0.0);
// } 


// =============================================================================
// R code to test.
// =============================================================================
// tmp2 = runif(1e8); paraSummationFloat(tmp2, maxCore = 1, grainSize = 100) / 
//   paraSummationFloat(tmp2, maxCore = 100, grainSize = 100) - 1








